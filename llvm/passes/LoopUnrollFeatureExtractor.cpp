#include "LoopUnrollFeatureExtractor.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"

using namespace llvm;

LoopUnrollFeatures llvm::extractLoopUnrollFeatures(Loop *L,
                                                   ScalarEvolution &SE) {

  LoopUnrollFeatures Features{};

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

      if (isa<PHINode>(&Inst)) {
        ++Features.numPhiNodes;
      }

      if (Inst.isTerminator()) {
        ++Features.numTerminatorInstructions;
      }

      if (isa<LoadInst>(&Inst)) {
        ++Features.numLoads;
      }

      if (isa<StoreInst>(&Inst)) {
        ++Features.numStores;
      }

      if (auto *BI = dyn_cast<BranchInst>(&Inst)) {

        ++Features.numBranches;

        if (BI->isConditional()) {
          ++Features.numConditionalBranches;
        }
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

  uint64_t MaxTripCount = SE.getSmallConstantMaxTripCount(L);

  Features.maxTripCountKnown = MaxTripCount != 0;
  Features.maxTripCount = MaxTripCount;

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