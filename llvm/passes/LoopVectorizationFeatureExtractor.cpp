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

struct LoopVectorizationFeatures {

  unsigned loopID;

  unsigned loopDepth;
  bool isInnermost;
  bool hasParentLoop;
  unsigned numSubLoops;
  unsigned numBasicBlocks;
  unsigned numExitingBlocks;

  unsigned numInstructions;
  unsigned numPhiNodes;
  unsigned numTerminatorInstructions;

  unsigned numLoads;
  unsigned numForwardContiguousLoads;
  unsigned numReverseContiguousLoads;
  unsigned numStridedLoads;
  unsigned numUnknownLoads;

  unsigned numStores;
  unsigned numForwardContiguousStores;
  unsigned numReverseContiguousStores;
  unsigned numStridedStores;
  unsigned numUnknownStores;

  unsigned numIntegerOps;
  unsigned numFloatOps;

  unsigned numBranches;
  unsigned numConditionalBranches;

  unsigned numCalls;

  bool tripCountKnown;
  uint64_t tripCount;

  bool maxTripCountKnown;
  uint64_t maxTripCount;

  double memoryOpRatio;
  double controlOverheadRatio;
  double arithmeticIntensity;

  bool hasReduction;
  unsigned numReductions;
};

class LoopVectorizationFeatureExtractor
    : public PassInfoMixin<LoopVectorizationFeatureExtractor> {

private:
  enum class MemoryAccessPattern {
    ForwardContiguous,
    ReverseContiguous,
    Strided,
    Unknown
  };
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

    if (StrideBytes == -static_cast<int64_t>(ElementSize)) {
      return MemoryAccessPattern::ReverseContiguous;
    }

    return MemoryAccessPattern::Strided;
  }

  bool isReductionPHI(PHINode *Phi, Loop *L, ScalarEvolution &SE) {
    if (!Phi->getParent() || !L->contains(Phi->getParent())) {
      return false;
    }
    if (SE.isSCEVable(Phi->getType())) {
      const SCEV *PhiSCEV = SE.getSCEV(Phi);

      if (isa<SCEVAddRecExpr>(PhiSCEV)) {
        return false;
      }
    }

    BasicBlock *Latch = L->getLoopLatch();

    if (!Latch) {
      return false;
    }

    int BackedgeIndex = Phi->getBasicBlockIndex(Latch);

    if (BackedgeIndex < 0) {
      return false;
    }

    Value *BackedgeValue = Phi->getIncomingValue(BackedgeIndex);

    auto *BinaryOp = dyn_cast<BinaryOperator>(BackedgeValue);

    if (!BinaryOp) {
      return false;
    }



    return BinaryOp->getOperand(0) == Phi || BinaryOp->getOperand(1) == Phi;
  }

  void extractLoopFeatures(Loop *L, ScalarEvolution &SE, const DataLayout &DL) {

    LoopVectorizationFeatures Features{};

    unsigned CurrentLoopID = LoopID++;
    Features.loopID = CurrentLoopID;

    attachLoopID(L, CurrentLoopID);
    Features.loopDepth = L->getLoopDepth();

    Features.isInnermost = L->isInnermost();

    Features.hasParentLoop = L->getParentLoop() != nullptr;

    Features.numSubLoops = L->getSubLoops().size();

    Features.numBasicBlocks = L->getNumBlocks();

    SmallVector<BasicBlock *, 4> ExitingBlocks;

    L->getExitingBlocks(ExitingBlocks);

    Features.numExitingBlocks = ExitingBlocks.size();

    unsigned numInstructions = 0;
    unsigned numPhiNodes = 0;
    unsigned numTerminatorInstructions = 0;

    unsigned numLoads = 0;
    unsigned numForwardContiguousLoads = 0;
    unsigned numReverseContiguousLoads = 0;
    unsigned numStridedLoads = 0;
    unsigned numUnknownLoads = 0;

    unsigned numStores = 0;
    unsigned numForwardContiguousStores = 0;
    unsigned numReverseContiguousStores = 0;
    unsigned numStridedStores = 0;
    unsigned numUnknownStores = 0;

    unsigned numIntegerOps = 0;
    unsigned numFloatOps = 0;

    bool hasReduction = false;
    unsigned numReductions = 0;

    unsigned numBranches = 0;
    unsigned numConditionalBranches = 0;

    unsigned numCalls = 0;

    for (BasicBlock *BB : L->blocks()) {

      for (Instruction &Inst : *BB) {

        ++numInstructions;

        if (auto *Phi = dyn_cast<PHINode>(&Inst)) {
          ++numPhiNodes;

          if (isReductionPHI(Phi, L, SE)) {
            hasReduction = true;
            ++numReductions;
          }
        }

        if (Inst.isTerminator()) {
          ++numTerminatorInstructions;
        }

        if (auto *Load = dyn_cast<LoadInst>(&Inst)) {
          ++numLoads;

          switch (classifyMemoryAccess(Load, SE, DL)) {
          case MemoryAccessPattern::ForwardContiguous:
            ++numForwardContiguousLoads;
            break;
          case MemoryAccessPattern::ReverseContiguous:
            ++numReverseContiguousLoads;
            break;
          case MemoryAccessPattern::Strided:
            ++numStridedLoads;
            break;
          case MemoryAccessPattern::Unknown:
            ++numUnknownLoads;
            break;
          }
        }

        if (auto *Store = dyn_cast<StoreInst>(&Inst)) {
          ++numStores;

          switch (classifyMemoryAccess(Store, SE, DL)) {
          case MemoryAccessPattern::ForwardContiguous:
            ++numForwardContiguousStores;
            break;
          case MemoryAccessPattern::ReverseContiguous:
            ++numReverseContiguousStores;
            break;
          case MemoryAccessPattern::Strided:
            ++numStridedStores;
            break;
          case MemoryAccessPattern::Unknown:
            ++numUnknownStores;
            break;
          }
        }

        if (auto *BI = dyn_cast<BranchInst>(&Inst)) {

          ++numBranches;

          if (BI->isConditional()) {
            ++numConditionalBranches;
          }
        }

        if (isa<CallBase>(&Inst)) {
          ++numCalls;
        }

        switch (Inst.getOpcode()) {

        case Instruction::Add:
        case Instruction::Sub:
        case Instruction::Mul:
        case Instruction::SDiv:
        case Instruction::UDiv:
        case Instruction::SRem:
        case Instruction::URem:

          ++numIntegerOps;
          break;

        case Instruction::FAdd:
        case Instruction::FSub:
        case Instruction::FMul:
        case Instruction::FDiv:
        case Instruction::FRem:

          ++numFloatOps;
          break;

        default:
          break;
        }
      }
    }

    Features.numInstructions = numInstructions;

    Features.numPhiNodes = numPhiNodes;

    Features.numTerminatorInstructions = numTerminatorInstructions;

    Features.numLoads = numLoads;

    Features.numStores = numStores;

    Features.numForwardContiguousLoads = numForwardContiguousLoads;

    Features.numReverseContiguousLoads = numReverseContiguousLoads;

    Features.numStridedLoads = numStridedLoads;

    Features.numUnknownLoads = numUnknownLoads;

    Features.numForwardContiguousStores = numForwardContiguousStores;

    Features.numReverseContiguousStores = numReverseContiguousStores;

    Features.numStridedStores = numStridedStores;

    Features.numUnknownStores = numUnknownStores;

    Features.numIntegerOps = numIntegerOps;

    Features.numFloatOps = numFloatOps;

    Features.hasReduction = hasReduction;

    Features.numReductions = numReductions;

    Features.numBranches = numBranches;

    Features.numConditionalBranches = numConditionalBranches;

    Features.numCalls = numCalls;

    uint64_t tripCount = SE.getSmallConstantTripCount(L);

    Features.tripCountKnown = tripCount != 0;

    Features.tripCount = tripCount;

    uint64_t maxTripCount = SE.getSmallConstantMaxTripCount(L);

    Features.maxTripCountKnown = maxTripCount != 0;

    Features.maxTripCount = maxTripCount;

    unsigned memoryOps = numLoads + numStores;

    unsigned arithmeticOps = numIntegerOps + numFloatOps;

    Features.memoryOpRatio =
        numInstructions > 0 ? static_cast<double>(memoryOps) / numInstructions
                            : 0.0;

    Features.controlOverheadRatio =
        numInstructions > 0 ? static_cast<double>(numBranches) / numInstructions
                            : 0.0;

    Features.arithmeticIntensity =
        memoryOps > 0 ? static_cast<double>(arithmeticOps) / memoryOps
                      : static_cast<double>(arithmeticOps);

    printInfo(Features);

    for (Loop *SubLoop : L->getSubLoops()) {

      extractLoopFeatures(SubLoop, SE, DL);
    }
  }

  void attachLoopID(Loop *L, unsigned LoopNumber) {
    LLVMContext &Ctx = L->getHeader()->getContext();

    SmallVector<Metadata *, 8> Operands;
    Operands.push_back(nullptr);

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

  void printInfo(const LoopVectorizationFeatures &Features) {

    errs() << "\n";
    errs() << "========================================\n";
    errs() << "Loop Vectorization Features\n";
    errs() << "========================================\n";

    errs() << "  loop_id: " << Features.loopID << "\n";

    errs() << "  loop_depth: " << Features.loopDepth << "\n";

    errs() << "  is_innermost: " << Features.isInnermost << "\n";

    errs() << "  has_parent_loop: " << Features.hasParentLoop << "\n";

    errs() << "  num_subloops: " << Features.numSubLoops << "\n";

    errs() << "  num_basic_blocks: " << Features.numBasicBlocks << "\n";

    errs() << "  num_exiting_blocks: " << Features.numExitingBlocks << "\n";

    errs() << "  num_instructions: " << Features.numInstructions << "\n";

    errs() << "  num_phi_nodes: " << Features.numPhiNodes << "\n";

    errs() << "  num_terminator_instructions: "
           << Features.numTerminatorInstructions << "\n";

    errs() << "  num_loads: " << Features.numLoads << "\n";

    errs() << "  num_stores: " << Features.numStores << "\n";

    errs() << "  num_forward_contiguous_loads: "
           << Features.numForwardContiguousLoads << "\n";

    errs() << "  num_reverse_contiguous_loads: "
           << Features.numReverseContiguousLoads << "\n";

    errs() << "  num_strided_loads: " << Features.numStridedLoads << "\n";

    errs() << "  num_unknown_loads: " << Features.numUnknownLoads << "\n";

    errs() << "  num_forward_contiguous_stores: "
           << Features.numForwardContiguousStores << "\n";

    errs() << "  num_reverse_contiguous_stores: "
           << Features.numReverseContiguousStores << "\n";

    errs() << "  num_strided_stores: " << Features.numStridedStores << "\n";

    errs() << "  num_unknown_stores: " << Features.numUnknownStores << "\n";

    errs() << "  num_integer_ops: " << Features.numIntegerOps << "\n";

    errs() << "  num_float_ops: " << Features.numFloatOps << "\n";

    errs() << "  has_reduction: " << Features.hasReduction << "\n";

    errs() << "  num_reductions: " << Features.numReductions << "\n";

    errs() << "  num_branches: " << Features.numBranches << "\n";

    errs() << "  num_conditional_branches: " << Features.numConditionalBranches
           << "\n";

    errs() << "  num_calls: " << Features.numCalls << "\n";

    errs() << "  trip_count_known: " << Features.tripCountKnown << "\n";

    errs() << "  trip_count: " << Features.tripCount << "\n";

    errs() << "  max_trip_count_known: " << Features.maxTripCountKnown << "\n";

    errs() << "  max_trip_count: " << Features.maxTripCount << "\n";

    errs() << "  memory_op_ratio: " << Features.memoryOpRatio << "\n";

    errs() << "  control_overhead_ratio: " << Features.controlOverheadRatio
           << "\n";

    errs() << "  arithmetic_intensity: " << Features.arithmeticIntensity
           << "\n";

    errs() << "========================================\n";
  }

public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {

    errs() << "\n--- LoopVectorizationFeatureExtractor Invoked ---\n";

    if (F.isDeclaration()) {

      return PreservedAnalyses::all();
    }

    LoopInfo &LI = AM.getResult<LoopAnalysis>(F);

    ScalarEvolution &SE = AM.getResult<ScalarEvolutionAnalysis>(F);

    errs() << "\nFunction: " << F.getName() << "\n";

    for (Loop *L : LI) {

      extractLoopFeatures(L, SE, F.getDataLayout());
    }

    return PreservedAnalyses::none();
  }
};

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {

  return {

      LLVM_PLUGIN_API_VERSION,

      "LoopVectorizationFeatureExtractor",

      LLVM_VERSION_STRING,

      [](PassBuilder &PB) {
        PB.registerPipelineParsingCallback(

            [](StringRef Name, FunctionPassManager &FPM,
               ArrayRef<PassBuilder::PipelineElement>) {
              if (Name == "loop-vectorization-feature-extractor-pass") {

                FPM.addPass(LoopVectorizationFeatureExtractor());

                return true;
              }

              return false;
            });
      }};
}