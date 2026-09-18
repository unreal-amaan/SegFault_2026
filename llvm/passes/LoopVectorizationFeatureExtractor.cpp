#include "LoopVectorizationFeatureExtractor.h"

#include "llvm/Analysis/ScalarEvolutionExpressions.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"

using namespace llvm;

namespace {

enum class MemoryAccessPattern {
  ForwardContiguous,
  ReverseContiguous,
  Strided,
  Unknown
};

MemoryAccessPattern classifyMemoryAccess(Instruction *Inst, ScalarEvolution &SE,
                                         const DataLayout &DL) {
  Value *Ptr = nullptr;
  Type *AccessType = nullptr;

  if (auto *Load = dyn_cast<LoadInst>(Inst)) {
    Ptr = Load->getPointerOperand();
    AccessType = Load->getType();
  } else if (auto *Store = dyn_cast<StoreInst>(Inst)) {
    Ptr = Store->getPointerOperand();
    AccessType = Store->getValueOperand()->getType();
  } else {
    return MemoryAccessPattern::Unknown;
  }

  const SCEV *PtrSCEV = SE.getSCEV(Ptr);

  auto *AddRec = dyn_cast<SCEVAddRecExpr>(PtrSCEV);

  if (!AddRec || !AddRec->isAffine())
    return MemoryAccessPattern::Unknown;

  const SCEV *Step = AddRec->getStepRecurrence(SE);

  auto *ConstantStep = dyn_cast<SCEVConstant>(Step);

  if (!ConstantStep)
    return MemoryAccessPattern::Unknown;

  int64_t StrideBytes = ConstantStep->getAPInt().getSExtValue();

  if (!AccessType->isSized())
    return MemoryAccessPattern::Unknown;

  uint64_t ElementSize = DL.getTypeAllocSize(AccessType);

  if (StrideBytes == static_cast<int64_t>(ElementSize))
    return MemoryAccessPattern::ForwardContiguous;

  if (StrideBytes == -static_cast<int64_t>(ElementSize))
    return MemoryAccessPattern::ReverseContiguous;

  return MemoryAccessPattern::Strided;
}

bool isReductionPHI(PHINode *Phi, Loop *L, ScalarEvolution &SE) {
  if (!Phi->getParent() || !L->contains(Phi->getParent()))
    return false;

  if (SE.isSCEVable(Phi->getType())) {
    const SCEV *PhiSCEV = SE.getSCEV(Phi);

    if (isa<SCEVAddRecExpr>(PhiSCEV))
      return false;
  }

  BasicBlock *Latch = L->getLoopLatch();

  if (!Latch)
    return false;

  int BackedgeIndex = Phi->getBasicBlockIndex(Latch);

  if (BackedgeIndex < 0)
    return false;

  Value *BackedgeValue = Phi->getIncomingValue(BackedgeIndex);

  auto *BinaryOp = dyn_cast<BinaryOperator>(BackedgeValue);

  if (!BinaryOp)
    return false;

  return BinaryOp->getOperand(0) == Phi || BinaryOp->getOperand(1) == Phi;
}

} // namespace

namespace llvm {

LoopVectorizationFeatures
extractLoopVectorizationFeatures(Loop *L, ScalarEvolution &SE,
                                 const DataLayout &DL) {

  LoopVectorizationFeatures Features{};

  Features.loopDepth = L->getLoopDepth();

  Features.isInnermost = L->isInnermost();

  Features.hasParentLoop = L->getParentLoop() != nullptr;

  Features.numSubLoops = L->getSubLoops().size();

  Features.numBasicBlocks = L->getNumBlocks();

  SmallVector<BasicBlock *, 4> ExitingBlocks;

  L->getExitingBlocks(ExitingBlocks);

  Features.numExitingBlocks = ExitingBlocks.size();

  for (BasicBlock *BB : L->blocks()) {

    for (Instruction &Inst : *BB) {

      ++Features.numInstructions;

      // ------------------------------------------------------------
      // PHI nodes
      // ------------------------------------------------------------

      if (auto *Phi = dyn_cast<PHINode>(&Inst)) {

        ++Features.numPhiNodes;

        if (isReductionPHI(Phi, L, SE)) {

          Features.hasReduction = true;

          ++Features.numReductions;
        }
      }

      // ------------------------------------------------------------
      // Terminators
      // ------------------------------------------------------------

      if (Inst.isTerminator())
        ++Features.numTerminatorInstructions;

      // ------------------------------------------------------------
      // Loads
      // ------------------------------------------------------------

      if (auto *Load = dyn_cast<LoadInst>(&Inst)) {

        ++Features.numLoads;

        switch (classifyMemoryAccess(Load, SE, DL)) {

        case MemoryAccessPattern::ForwardContiguous:
          ++Features.numForwardContiguousLoads;
          break;

        case MemoryAccessPattern::ReverseContiguous:
          ++Features.numReverseContiguousLoads;
          break;

        case MemoryAccessPattern::Strided:
          ++Features.numStridedLoads;
          break;

        case MemoryAccessPattern::Unknown:
          ++Features.numUnknownLoads;
          break;
        }
      }

      // ------------------------------------------------------------
      // Stores
      // ------------------------------------------------------------

      if (auto *Store = dyn_cast<StoreInst>(&Inst)) {

        ++Features.numStores;

        switch (classifyMemoryAccess(Store, SE, DL)) {

        case MemoryAccessPattern::ForwardContiguous:
          ++Features.numForwardContiguousStores;
          break;

        case MemoryAccessPattern::ReverseContiguous:
          ++Features.numReverseContiguousStores;
          break;

        case MemoryAccessPattern::Strided:
          ++Features.numStridedStores;
          break;

        case MemoryAccessPattern::Unknown:
          ++Features.numUnknownStores;
          break;
        }
      }

      // ------------------------------------------------------------
      // Branches
      // ------------------------------------------------------------

      if (auto *BI = dyn_cast<BranchInst>(&Inst)) {

        ++Features.numBranches;

        if (BI->isConditional())
          ++Features.numConditionalBranches;
      }

      // ------------------------------------------------------------
      // Calls
      // ------------------------------------------------------------

      if (isa<CallBase>(&Inst))
        ++Features.numCalls;

      // ------------------------------------------------------------
      // Arithmetic
      // ------------------------------------------------------------

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

  // ------------------------------------------------------------
  // Trip count
  // ------------------------------------------------------------

  uint64_t TripCount = SE.getSmallConstantTripCount(L);

  Features.tripCountKnown = TripCount != 0;

  Features.tripCount = TripCount;

  uint64_t MaxTripCount = SE.getSmallConstantMaxTripCount(L);

  Features.maxTripCountKnown = MaxTripCount != 0;

  Features.maxTripCount = MaxTripCount;

  // ------------------------------------------------------------
  // Derived features
  // ------------------------------------------------------------

  unsigned MemoryOps = Features.numLoads + Features.numStores;

  unsigned ArithmeticOps = Features.numIntegerOps + Features.numFloatOps;

  Features.memoryOpRatio =
      Features.numInstructions > 0
          ? static_cast<double>(MemoryOps) / Features.numInstructions
          : 0.0;

  Features.controlOverheadRatio =
      Features.numInstructions > 0
          ? static_cast<double>(Features.numBranches) / Features.numInstructions
          : 0.0;

  Features.arithmeticIntensity =
      MemoryOps > 0 ? static_cast<double>(ArithmeticOps) / MemoryOps
                    : static_cast<double>(ArithmeticOps);

  return Features;
}

} // namespace llvm