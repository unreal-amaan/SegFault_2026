#include "LoopTilingFeatureExtractor.h"

#include "llvm/Analysis/ScalarEvolutionExpressions.h"

#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
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

const SCEVAddRecExpr *findAddRecForLoop(const SCEV *S, Loop *L) {

  if (auto *AddRec = dyn_cast<SCEVAddRecExpr>(S)) {

    if (AddRec->getLoop() == L)
      return AddRec;
  }

  for (const SCEV *Operand : S->operands()) {

    if (const SCEVAddRecExpr *AddRec = findAddRecForLoop(Operand, L)) {

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

  const SCEV *PtrSCEV = SE.getSCEV(Ptr);

  const SCEVAddRecExpr *AddRec = findAddRecForLoop(PtrSCEV, L);

  if (!AddRec || !AddRec->isAffine())
    return MemoryAccessPattern::Unknown;

  const SCEV *Step = AddRec->getStepRecurrence(SE);

  auto *ConstantStep = dyn_cast<SCEVConstant>(Step);

  if (!ConstantStep)
    return MemoryAccessPattern::Unknown;

  int64_t StrideBytes = ConstantStep->getAPInt().getSExtValue();

  Type *AccessType = nullptr;

  if (auto *Load = dyn_cast<LoadInst>(Inst)) {

    AccessType = Load->getType();

  } else {

    auto *Store = cast<StoreInst>(Inst);

    AccessType = Store->getValueOperand()->getType();
  }

  if (!AccessType || !AccessType->isSized())
    return MemoryAccessPattern::Unknown;

  uint64_t ElementSize = DL.getTypeAllocSize(AccessType);

  if (StrideBytes == static_cast<int64_t>(ElementSize)) {

    return MemoryAccessPattern::ForwardContiguous;
  }

  if (StrideBytes == -static_cast<int64_t>(ElementSize)) {

    return MemoryAccessPattern::ReverseContiguous;
  }

  return MemoryAccessPattern::Strided;
}

} // namespace

LoopTilingFeatures llvm::extractLoopTilingFeatures(Loop *L, ScalarEvolution &SE,
                                                   const DataLayout &DL) {

  LoopTilingFeatures Features{};

  if (!L)
    return Features;

  // ------------------------------------------------------------
  // Loop structure
  // ------------------------------------------------------------

  Features.loopDepth = L->getLoopDepth();

  Features.isInnermost = L->isInnermost();

  Features.hasParentLoop = L->getParentLoop() != nullptr;

  Features.numSubLoops = L->getSubLoops().size();

  Features.numBasicBlocks = L->getNumBlocks();

  SmallVector<BasicBlock *, 4> ExitingBlocks;

  L->getExitingBlocks(ExitingBlocks);

  Features.numExitingBlocks = ExitingBlocks.size();

  // ------------------------------------------------------------
  // Loop body
  // ------------------------------------------------------------

  for (BasicBlock *BB : L->blocks()) {

    for (Instruction &Inst : *BB) {

      ++Features.numInstructions;

      if (isa<PHINode>(&Inst))
        ++Features.numPhiNodes;

      if (Inst.isTerminator())
        ++Features.numTerminatorInstructions;

      // ----------------------------------------------------------
      // Loads
      // ----------------------------------------------------------

      if (auto *Load = dyn_cast<LoadInst>(&Inst)) {

        ++Features.numLoads;

        switch (classifyMemoryAccess(Load, L, SE, DL)) {

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

      // ----------------------------------------------------------
      // Stores
      // ----------------------------------------------------------

      if (auto *Store = dyn_cast<StoreInst>(&Inst)) {

        ++Features.numStores;

        switch (classifyMemoryAccess(Store, L, SE, DL)) {

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

      // ----------------------------------------------------------
      // Branches
      // ----------------------------------------------------------

      if (auto *BI = dyn_cast<BranchInst>(&Inst)) {

        ++Features.numBranches;

        if (BI->isConditional())
          ++Features.numConditionalBranches;
      }

      // ----------------------------------------------------------
      // Calls
      // ----------------------------------------------------------

      if (isa<CallBase>(&Inst))
        ++Features.numCalls;

      // ----------------------------------------------------------
      // Arithmetic
      // ----------------------------------------------------------

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