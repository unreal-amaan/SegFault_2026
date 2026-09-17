#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Plugins/PassPlugin.h"

#include "llvm/Support/raw_ostream.h"

    using namespace llvm;

unsigned LoopID = 0;

struct LoopTilingFeatures {

  unsigned loopID;

  unsigned loopDepth;
  bool isInnermost;
  unsigned numSubLoops;

  bool tripCountKnown;
  uint64_t tripCount;

  bool maxTripCountKnown;
  uint64_t maxTripCount;

  unsigned numInstructions;
  unsigned numLoads;
  unsigned numStores;

  unsigned numIntegerOps;
  unsigned numFloatOps;

  unsigned numForwardContiguousLoads;
  unsigned numStridedLoads;

  unsigned numForwardContiguousStores;
  unsigned numStridedStores;
};

class LoopTilingFeatureExtractor
    : public PassInfoMixin<LoopTilingFeatureExtractor> {

private:
  enum class MemoryAccessPattern {
    ForwardContiguous,
    ReverseContiguous,
    Strided,
    Unknown
  };

  /*
   * Recursively search a SCEV expression for an AddRec belonging
   * to the loop currently being analyzed.
   *
   * For example, a pointer SCEV may look like:
   *
   *   outer_expression + AddRec(inner_loop)
   *
   * rather than being an AddRec itself.
   */
  const SCEVAddRecExpr *findAddRecForLoop(const SCEV *S, Loop *L) {

    if (auto *AddRec = dyn_cast<SCEVAddRecExpr>(S)) {

      if (AddRec->getLoop() == L) {
        return AddRec;
      }
    }

    for (const SCEV *Operand : S->operands()) {

      if (const SCEVAddRecExpr *AddRec =
              findAddRecForLoop(Operand, L)) {

        return AddRec;
      }
    }

    return nullptr;
  }

  MemoryAccessPattern classifyMemoryAccess(Instruction *Inst, Loop *L,
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

    /*
     * Obtain the scalar-evolution representation of the address.
     */
    const SCEV *PtrSCEV = SE.getSCEV(Ptr);

    /*
     * The AddRec does not necessarily have to be the top-level
     * SCEV expression. Search recursively for the AddRec associated
     * with the current loop.
     */
    const SCEVAddRecExpr *AddRec = findAddRecForLoop(PtrSCEV, L);

    if (!AddRec || !AddRec->isAffine()) {

      return MemoryAccessPattern::Unknown;
    }

    /*
     * Extract the loop-dependent step.
     */
    const SCEV *Step = AddRec->getStepRecurrence(SE);

    auto *ConstantStep = dyn_cast<SCEVConstant>(Step);

    if (!ConstantStep) {

      return MemoryAccessPattern::Unknown;
    }

    int64_t StrideBytes = ConstantStep->getAPInt().getSExtValue();

    /*
     * Determine the size of the accessed element.
     */
    Type *AccessType = nullptr;

    if (auto *Load = dyn_cast<LoadInst>(Inst)) {

      AccessType = Load->getType();

    } else {

      auto *Store = cast<StoreInst>(Inst);

      AccessType = Store->getValueOperand()->getType();
    }

    if (!AccessType || !AccessType->isSized()) {

      return MemoryAccessPattern::Unknown;
    }

    uint64_t ElementSize = DL.getTypeAllocSize(AccessType);

    /*
     * Sequential access:
     *
     *   A[i]     -> +element_size
     *   A[i - 1] -> -element_size
     */
    if (StrideBytes == static_cast<int64_t>(ElementSize)) {

      return MemoryAccessPattern::ForwardContiguous;
    }

    if (StrideBytes == -static_cast<int64_t>(ElementSize)) {

      return MemoryAccessPattern::ReverseContiguous;
    }

    /*
     * Any other constant stride is considered strided.
     */
    return MemoryAccessPattern::Strided;
  }

  void attachLoopID(Loop *L, unsigned LoopNumber) {

    LLVMContext &Ctx = L->getHeader()->getContext();

    SmallVector<Metadata *, 8> Operands;

    /*
     * Loop ID metadata is self-referential.
     */
    Operands.push_back(nullptr);

    /*
     * Preserve existing loop metadata.
     */
    if (MDNode *ExistingLoopID = L->getLoopID()) {

      for (unsigned I = 1; I < ExistingLoopID->getNumOperands(); ++I) {

        Operands.push_back(ExistingLoopID->getOperand(I));
      }
    }

    Metadata *LoopIDOperands[] = {

        MDString::get(Ctx, "compiler_cost_model.loop_id"),

        ConstantAsMetadata::get(
            ConstantInt::get(Type::getInt32Ty(Ctx), LoopNumber))};

    Operands.push_back(MDNode::get(Ctx, LoopIDOperands));

    MDNode *NewLoopID = MDNode::getDistinct(Ctx, Operands);

    NewLoopID->replaceOperandWith(0, NewLoopID);

    L->setLoopID(NewLoopID);
  }

  void extractLoopFeatures(Loop *L, ScalarEvolution &SE, const DataLayout &DL) {

    LoopTilingFeatures Features{};

    Features.loopID = LoopID++;

    attachLoopID(L, Features.loopID);

    /*
     * Basic loop structure.
     */
    Features.loopDepth = L->getLoopDepth();

    Features.isInnermost = L->isInnermost();

    Features.numSubLoops = L->getSubLoops().size();

    /*
     * Trip count.
     */
    uint64_t TripCount = SE.getSmallConstantTripCount(L);

    Features.tripCountKnown = TripCount != 0;

    Features.tripCount = TripCount;

    uint64_t MaxTripCount = SE.getSmallConstantMaxTripCount(L);

    Features.maxTripCountKnown = MaxTripCount != 0;

    Features.maxTripCount = MaxTripCount;

    /*
     * Instruction and memory-operation counts.
     */
    unsigned NumInstructions = 0;
    unsigned NumLoads = 0;
    unsigned NumStores = 0;

    unsigned NumIntegerOps = 0;
    unsigned NumFloatOps = 0;

    unsigned NumForwardContiguousLoads = 0;
    unsigned NumStridedLoads = 0;

    unsigned NumForwardContiguousStores = 0;
    unsigned NumStridedStores = 0;

    /*
     * Analyze all instructions belonging to this loop.
     */
    for (BasicBlock *BB : L->blocks()) {

      for (Instruction &Inst : *BB) {

        ++NumInstructions;

        /*
         * Loads.
         */
        if (auto *Load = dyn_cast<LoadInst>(&Inst)) {

          ++NumLoads;

          switch (classifyMemoryAccess(Load, L, SE, DL)) {

          case MemoryAccessPattern::ForwardContiguous:

            ++NumForwardContiguousLoads;
            break;

          case MemoryAccessPattern::Strided:

            ++NumStridedLoads;
            break;

          default:

            break;
          }
        }

        /*
         * Stores.
         */
        if (auto *Store = dyn_cast<StoreInst>(&Inst)) {

          ++NumStores;

          switch (classifyMemoryAccess(Store, L, SE, DL)) {

          case MemoryAccessPattern::ForwardContiguous:

            ++NumForwardContiguousStores;
            break;

          case MemoryAccessPattern::Strided:

            ++NumStridedStores;
            break;

          default:

            break;
          }
        }

        /*
         * Integer arithmetic.
         */
        switch (Inst.getOpcode()) {

        case Instruction::Add:
        case Instruction::Sub:
        case Instruction::Mul:
        case Instruction::SDiv:
        case Instruction::UDiv:
        case Instruction::SRem:
        case Instruction::URem:

          ++NumIntegerOps;
          break;

        /*
         * Floating-point arithmetic.
         */
        case Instruction::FAdd:
        case Instruction::FSub:
        case Instruction::FMul:
        case Instruction::FDiv:
        case Instruction::FRem:

          ++NumFloatOps;
          break;

        default:

          break;
        }
      }
    }

    /*
     * Store extracted features.
     */
    Features.numInstructions = NumInstructions;

    Features.numLoads = NumLoads;

    Features.numStores = NumStores;

    Features.numIntegerOps = NumIntegerOps;

    Features.numFloatOps = NumFloatOps;

    Features.numForwardContiguousLoads = NumForwardContiguousLoads;

    Features.numStridedLoads = NumStridedLoads;

    Features.numForwardContiguousStores = NumForwardContiguousStores;

    Features.numStridedStores = NumStridedStores;

    printInfo(Features);

    /*
     * Recursively process nested loops.
     */
    for (Loop *SubLoop : L->getSubLoops()) {

      extractLoopFeatures(SubLoop, SE, DL);
    }
  }

  void printInfo(const LoopTilingFeatures &Features) {

    errs() << Features.loopID << "," << Features.loopDepth << ","
           << Features.isInnermost << "," << Features.numSubLoops << ","
           << Features.tripCountKnown << "," << Features.tripCount << ","
           << Features.maxTripCountKnown << "," << Features.maxTripCount << ","
           << Features.numInstructions << "," << Features.numLoads << ","
           << Features.numStores << "," << Features.numIntegerOps << ","
           << Features.numFloatOps << "," << Features.numForwardContiguousLoads
           << "," << Features.numStridedLoads << ","
           << Features.numForwardContiguousStores << ","
           << Features.numStridedStores << "\n";
  }

public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {

    errs() << "\n--- LoopTilingFeatureExtractor Invoked ---\n";

    if (F.isDeclaration()) {

      return PreservedAnalyses::all();
    }

    LoopInfo &LI = AM.getResult<LoopAnalysis>(F);

    ScalarEvolution &SE = AM.getResult<ScalarEvolutionAnalysis>(F);

    const DataLayout &DL = F.getParent()->getDataLayout();

    /*
     * LoopInfo contains top-level loops.
     * extractLoopFeatures() recursively handles
     * nested loops.
     */
    for (Loop *L : LI) {

      extractLoopFeatures(L, SE, DL);
    }

    return PreservedAnalyses::all();
  }
};

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {

  return {LLVM_PLUGIN_API_VERSION, "LoopTilingFeatureExtractor",
          LLVM_VERSION_STRING,

          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "loop-tiling-features") {

                    FPM.addPass(LoopTilingFeatureExtractor());

                    return true;
                  }

                  return false;
                });
          }};
}
