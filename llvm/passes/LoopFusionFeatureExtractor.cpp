#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"

#include "llvm/IR/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"

#include "llvm/Analysis/ValueTracking.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Plugins/PassPlugin.h"

#include "llvm/Support/raw_ostream.h"

#include <cstdint>
#include <map>
#include <set>
#include <utility>
#include <vector>

    using namespace llvm;

struct LoopFeatures {

  uint64_t tripCount = 0;
  bool tripCountKnown = false;

  unsigned numInstructions = 0;
  unsigned numLoads = 0;
  unsigned numStores = 0;

  unsigned numForwardContiguousLoads = 0;
  unsigned numForwardContiguousStores = 0;

  unsigned numIntegerOps = 0;
  unsigned numFloatOps = 0;

  unsigned numBranches = 0;
  unsigned numCalls = 0;

  unsigned numMemoryOps() const { return numLoads + numStores; }

  unsigned numContiguousMemoryOps() const {
    return numForwardContiguousLoads + numForwardContiguousStores;
  }

  double memoryOpRatio() const {
    if (numInstructions == 0)
      return 0.0;

    return static_cast<double>(numMemoryOps()) /
           static_cast<double>(numInstructions);
  }
};

struct LoopFusionFeatures {

  unsigned loop1ID = 0;
  unsigned loop2ID = 0;

  LoopFeatures loop1;
  LoopFeatures loop2;

  bool loopsAdjacent = false;

  bool tripCountEqual = false;

  double tripCountRatio = 0.0;

  unsigned sharedMemoryObjects = 0;

  unsigned loop1StoresReadByLoop2 = 0;
  unsigned loop2StoresReadByLoop1 = 0;

  unsigned combinedMemoryOps = 0;
};

class LoopFusionFeatureExtractor
    : public PassInfoMixin<LoopFusionFeatureExtractor> {

private:
  // Maps the actual LLVM Loop object to the persistent
  // compiler_cost_model.loop_id assigned to it.
  std::map<Loop *, unsigned> LoopIDs;

  enum class MemoryAccessPattern { ForwardContiguous, Unknown };

  MemoryAccessPattern classifyMemoryAccess(Instruction *Inst,
                                           ScalarEvolution &SE,
                                           const DataLayout &DL) {

    Value *Ptr = nullptr;

    if (auto *Load = dyn_cast<LoadInst>(Inst)) {

      Ptr = Load->getPointerOperand();

    } else if (auto *Store = dyn_cast<StoreInst>(Inst)) {

      Ptr = Store->getPointerOperand();

    } else {

      return MemoryAccessPattern::Unknown;
    }

    const SCEV *PtrSCEV = SE.getSCEV(Ptr);

    auto *AddRec = dyn_cast<SCEVAddRecExpr>(PtrSCEV);

    if (!AddRec || !AddRec->isAffine()) {

      return MemoryAccessPattern::Unknown;
    }

    const SCEV *Step = AddRec->getStepRecurrence(SE);

    auto *ConstantStep = dyn_cast<SCEVConstant>(Step);

    if (!ConstantStep) {

      return MemoryAccessPattern::Unknown;
    }

    int64_t StrideBytes = ConstantStep->getAPInt().getSExtValue();

    Type *AccessType = nullptr;

    if (auto *Load = dyn_cast<LoadInst>(Inst)) {

      AccessType = Load->getType();

    } else {

      AccessType = cast<StoreInst>(Inst)->getValueOperand()->getType();
    }

    if (!AccessType->isSized()) {

      return MemoryAccessPattern::Unknown;
    }

    uint64_t ElementSize = DL.getTypeAllocSize(AccessType);

    if (StrideBytes == static_cast<int64_t>(ElementSize)) {

      return MemoryAccessPattern::ForwardContiguous;
    }

    return MemoryAccessPattern::Unknown;
  }

  LoopFeatures extractLoopFeatures(Loop *L, ScalarEvolution &SE,
                                   const DataLayout &DL) {

    LoopFeatures Features;

    for (BasicBlock *BB : L->blocks()) {

      for (Instruction &Inst : *BB) {

        ++Features.numInstructions;

        if (auto *Load = dyn_cast<LoadInst>(&Inst)) {

          ++Features.numLoads;

          if (classifyMemoryAccess(Load, SE, DL) ==
              MemoryAccessPattern::ForwardContiguous) {

            ++Features.numForwardContiguousLoads;
          }
        }

        if (auto *Store = dyn_cast<StoreInst>(&Inst)) {

          ++Features.numStores;

          if (classifyMemoryAccess(Store, SE, DL) ==
              MemoryAccessPattern::ForwardContiguous) {

            ++Features.numForwardContiguousStores;
          }
        }

        if (isa<BranchInst>(&Inst)) {

          ++Features.numBranches;
        }

        if (isa<CallBase>(&Inst)) {

          ++Features.numCalls;
        }

        switch (Inst.getOpcode()) {

        case Instruction::Add:
        case Instruction::Sub:
        case Instruction::Mul:
        case Instruction::SDiv:
        case Instruction::UDiv:
        case Instruction::SRem:
        case Instruction::URem:

          ++Features.numIntegerOps;
          break;

        case Instruction::FAdd:
        case Instruction::FSub:
        case Instruction::FMul:
        case Instruction::FDiv:
        case Instruction::FRem:

          ++Features.numFloatOps;
          break;

        default:
          break;
        }
      }
    }

    uint64_t TripCount = SE.getSmallConstantTripCount(L);

    Features.tripCountKnown = TripCount != 0;

    Features.tripCount = TripCount;

    return Features;
  }

  bool isAdjacent(Loop *L1, Loop *L2) {

    SmallVector<BasicBlock *, 4> ExitingBlocks;

    L1->getExitingBlocks(ExitingBlocks);

    BasicBlock *L2Header = L2->getHeader();

    for (BasicBlock *ExitBB : ExitingBlocks) {

      SmallVector<BasicBlock *, 4> Worklist;

      SmallPtrSet<BasicBlock *, 8> Visited;

      Worklist.push_back(ExitBB);

      while (!Worklist.empty()) {

        BasicBlock *BB = Worklist.pop_back_val();

        if (!Visited.insert(BB).second)
          continue;

        for (BasicBlock *Successor : successors(BB)) {

          if (Successor == L2Header)
            return true;

          // Do not walk into another loop.
          if (Successor->getParent() != L2->getHeader()->getParent()) {

            continue;
          }

          if (L2->contains(Successor))
            continue;

          Worklist.push_back(Successor);
        }
      }
    }

    return false;
  }

  std::vector<Value *> getMemoryObjects(Loop *L, bool StoresOnly,
                                        bool LoadsOnly) {

    std::vector<Value *> Objects;

    for (BasicBlock *BB : L->blocks()) {

      for (Instruction &Inst : *BB) {

        Value *Ptr = nullptr;

        if (auto *Load = dyn_cast<LoadInst>(&Inst)) {

          if (StoresOnly)
            continue;

          Ptr = Load->getPointerOperand();

        } else if (auto *Store = dyn_cast<StoreInst>(&Inst)) {

          if (LoadsOnly)
            continue;

          Ptr = Store->getPointerOperand();

        } else {

          continue;
        }

        Value *Object = getUnderlyingObject(Ptr);

        Objects.push_back(Object);
      }
    }

    return Objects;
  }

  std::set<Value *> getUniqueMemoryObjects(Loop *L) {

    std::set<Value *> Objects;

    for (BasicBlock *BB : L->blocks()) {

      for (Instruction &Inst : *BB) {

        Value *Ptr = nullptr;

        if (auto *Load = dyn_cast<LoadInst>(&Inst)) {

          Ptr = Load->getPointerOperand();

        } else if (auto *Store = dyn_cast<StoreInst>(&Inst)) {

          Ptr = Store->getPointerOperand();

        } else {

          continue;
        }

        Objects.insert(getUnderlyingObject(Ptr));
      }
    }

    return Objects;
  }

  unsigned countStoresReadByOtherLoop(Loop *Producer, Loop *Consumer) {

    std::set<Value *> StoredObjects;

    for (BasicBlock *BB : Producer->blocks()) {

      for (Instruction &Inst : *BB) {

        auto *Store = dyn_cast<StoreInst>(&Inst);

        if (!Store)
          continue;

        Value *Object = getUnderlyingObject(Store->getPointerOperand());

        StoredObjects.insert(Object);
      }
    }

    unsigned Count = 0;

    for (BasicBlock *BB : Consumer->blocks()) {

      for (Instruction &Inst : *BB) {

        auto *Load = dyn_cast<LoadInst>(&Inst);

        if (!Load)
          continue;

        Value *Object = getUnderlyingObject(Load->getPointerOperand());

        if (StoredObjects.count(Object)) {

          ++Count;
        }
      }
    }

    return Count;
  }

  LoopFusionFeatures analyzeLoopPair(Loop *L1, Loop *L2, ScalarEvolution &SE,
                                     const DataLayout &DL, unsigned ID1,
                                     unsigned ID2) {

    LoopFusionFeatures Features;

    Features.loop1ID = ID1;
    Features.loop2ID = ID2;

    Features.loop1 = extractLoopFeatures(L1, SE, DL);

    Features.loop2 = extractLoopFeatures(L2, SE, DL);

    Features.loopsAdjacent = isAdjacent(L1, L2);

    if (Features.loop1.tripCountKnown && Features.loop2.tripCountKnown) {

      Features.tripCountEqual =
          Features.loop1.tripCount == Features.loop2.tripCount;

      if (Features.loop2.tripCount != 0) {

        Features.tripCountRatio =
            static_cast<double>(Features.loop1.tripCount) /
            static_cast<double>(Features.loop2.tripCount);
      }
    }

    std::set<Value *> Objects1 = getUniqueMemoryObjects(L1);

    std::set<Value *> Objects2 = getUniqueMemoryObjects(L2);

    for (Value *Object : Objects1) {

      if (Objects2.count(Object)) {

        ++Features.sharedMemoryObjects;
      }
    }

    Features.loop1StoresReadByLoop2 = countStoresReadByOtherLoop(L1, L2);

    Features.loop2StoresReadByLoop1 = countStoresReadByOtherLoop(L2, L1);

    Features.combinedMemoryOps =
        Features.loop1.numMemoryOps() + Features.loop2.numMemoryOps();

    return Features;
  }

  void printLoopFeatures(StringRef Prefix, const LoopFeatures &Features) {

    errs() << "  " << Prefix << "_trip_count_known: " << Features.tripCountKnown
           << "\n";

    errs() << "  " << Prefix << "_trip_count: " << Features.tripCount << "\n";

    errs() << "  " << Prefix
           << "_num_instructions: " << Features.numInstructions << "\n";

    errs() << "  " << Prefix << "_num_loads: " << Features.numLoads << "\n";

    errs() << "  " << Prefix << "_num_stores: " << Features.numStores << "\n";

    errs() << "  " << Prefix << "_num_forward_contiguous_loads: "
           << Features.numForwardContiguousLoads << "\n";

    errs() << "  " << Prefix << "_num_forward_contiguous_stores: "
           << Features.numForwardContiguousStores << "\n";

    errs() << "  " << Prefix << "_num_integer_ops: " << Features.numIntegerOps
           << "\n";

    errs() << "  " << Prefix << "_num_float_ops: " << Features.numFloatOps
           << "\n";

    errs() << "  " << Prefix << "_num_branches: " << Features.numBranches
           << "\n";

    errs() << "  " << Prefix << "_num_calls: " << Features.numCalls << "\n";

    errs() << "  " << Prefix << "_memory_op_ratio: " << Features.memoryOpRatio()
           << "\n";

    errs() << "  " << Prefix
           << "_contiguous_memory_ops: " << Features.numContiguousMemoryOps()
           << "\n";
  }

  void printInfo(const LoopFusionFeatures &Features) {

    errs() << "\n";
    errs() << "========================================\n";
    errs() << "Loop Fusion Features\n";
    errs() << "========================================\n";

    errs() << "  loop1_id: " << Features.loop1ID << "\n";

    errs() << "  loop2_id: " << Features.loop2ID << "\n";

    printLoopFeatures("loop1", Features.loop1);

    printLoopFeatures("loop2", Features.loop2);

    errs() << "  loops_adjacent: " << Features.loopsAdjacent << "\n";

    errs() << "  trip_count_equal: " << Features.tripCountEqual << "\n";

    errs() << "  trip_count_ratio: " << Features.tripCountRatio << "\n";

    errs() << "  shared_memory_objects: " << Features.sharedMemoryObjects
           << "\n";

    errs() << "  loop1_stores_read_by_loop2: "
           << Features.loop1StoresReadByLoop2 << "\n";

    errs() << "  loop2_stores_read_by_loop1: "
           << Features.loop2StoresReadByLoop1 << "\n";

    errs() << "  combined_memory_ops: " << Features.combinedMemoryOps << "\n";

    errs() << "========================================\n";
  }

  void assignLoopID(Loop *L, unsigned ID) {

    LLVMContext &Ctx = L->getHeader()->getContext();

    MDNode *OldLoopID = L->getLoopID();

    SmallVector<Metadata *, 8> MDs;

    // First operand must refer to
    // the loop metadata node itself.
    MDs.push_back(nullptr);

    // Preserve existing loop metadata.
    if (OldLoopID) {

      for (unsigned I = 1; I < OldLoopID->getNumOperands(); ++I) {

        MDs.push_back(OldLoopID->getOperand(I));
      }
    }

    Metadata *LoopIDMD[] = {

        MDString::get(Ctx, "compiler_cost_model.loop_id"),

        ConstantAsMetadata::get(ConstantInt::get(Type::getInt32Ty(Ctx), ID))};

    MDs.push_back(MDNode::get(Ctx, LoopIDMD));

    MDNode *NewLoopID = MDNode::getDistinct(Ctx, MDs);

    NewLoopID->replaceOperandWith(0, NewLoopID);

    L->setLoopID(NewLoopID);
  }

public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {

    errs() << "\n--- LoopFusionFeatureExtractor Invoked ---\n";

    if (F.isDeclaration()) {

      return PreservedAnalyses::all();
    }

    // Clear the mapping for this function.
    LoopIDs.clear();

    LoopInfo &LI = AM.getResult<LoopAnalysis>(F);

    ScalarEvolution &SE = AM.getResult<ScalarEvolutionAnalysis>(F);

    errs() << "\nFunction: " << F.getName() << "\n";

    SmallVector<Loop *, 8> Loops;

    unsigned NextLoopID = 0;

    // We currently consider only top-level loops.
    for (Loop *L : LI) {

      if (L->getParentLoop() == nullptr) {

        Loops.push_back(L);
      }
    }

    // Assign the persistent loop ID once.
    //
    // The same ID is:
    //   1. attached to LLVM loop metadata
    //   2. stored in LoopIDs
    //   3. used in every fusion candidate
    for (Loop *L : Loops) {

      unsigned ID = NextLoopID++;

      assignLoopID(L, ID);

      LoopIDs[L] = ID;

      errs() << "Assigned loop ID " << ID
             << " to loop header: " << L->getHeader()->getName() << "\n";
    }

    // Generate features for every adjacent pair.
    //
    // IMPORTANT:
    // We retrieve the IDs from LoopIDs rather than
    // generating new IDs here.
    for (size_t I = 0; I < Loops.size(); ++I) {

      for (size_t J = I + 1; J < Loops.size(); ++J) {

        Loop *L1 = Loops[I];

        Loop *L2 = Loops[J];

        // Normalize the pair so L1 comes before L2.
        if (isAdjacent(L2, L1)) {

          std::swap(L1, L2);
        }

        if (!isAdjacent(L1, L2))
          continue;

        auto ID1It = LoopIDs.find(L1);

        auto ID2It = LoopIDs.find(L2);

        if (ID1It == LoopIDs.end() || ID2It == LoopIDs.end()) {

          errs() << "ERROR: missing loop ID "
                    "for fusion candidate\n";

          continue;
        }

        unsigned Loop1ID = ID1It->second;

        unsigned Loop2ID = ID2It->second;

        LoopFusionFeatures Features =
            analyzeLoopPair(L1, L2, SE, F.getDataLayout(), Loop1ID, Loop2ID);

        printInfo(Features);
      }
    }

    return PreservedAnalyses::none();
  }
};

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {

  return {

      LLVM_PLUGIN_API_VERSION,

      "LoopFusionFeatureExtractor",

      LLVM_VERSION_STRING,

      [](PassBuilder &PB) {
        PB.registerPipelineParsingCallback(

            [](StringRef Name, FunctionPassManager &FPM,
               ArrayRef<PassBuilder::PipelineElement>) {
              if (Name == "loop-fusion-feature-extractor-pass") {

                FPM.addPass(LoopFusionFeatureExtractor());

                return true;
              }

              return false;
            });
      }};
}