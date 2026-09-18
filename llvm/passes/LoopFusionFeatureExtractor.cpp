#include "LoopFusionFeatureExtractor.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"

using namespace llvm;

LoopFusionFeatures llvm::extractLoopFusionFeatures(Loop *L1, Loop *L2,
                                                   ScalarEvolution &SE) {

  LoopFusionFeatures F{};

  if (!L1 || !L2)
    return F;

  F.loop1ID = L1->getLoopDepth();
  F.loop2ID = L2->getLoopDepth();

  auto collect = [&](Loop *L, bool &TripKnown, uint64_t &TripCount,
                     unsigned &Instructions, unsigned &Loads, unsigned &Stores,
                     unsigned &ForwardLoads, unsigned &ForwardStores,
                     unsigned &IntegerOps, unsigned &FloatOps,
                     unsigned &Branches, unsigned &Calls, double &MemoryRatio,
                     unsigned &ContiguousOps) {
    for (BasicBlock *BB : L->blocks()) {
      for (Instruction &I : *BB) {

        ++Instructions;

        if (isa<LoadInst>(&I)) {
          ++Loads;
          ++ContiguousOps;
          ++ForwardLoads;
        }

        if (isa<StoreInst>(&I)) {
          ++Stores;
          ++ContiguousOps;
          ++ForwardStores;
        }

        if (auto *BI = dyn_cast<BranchInst>(&I))
          ++Branches;

        if (isa<CallBase>(&I))
          ++Calls;

        switch (I.getOpcode()) {

        case Instruction::Add:
        case Instruction::Sub:
        case Instruction::Mul:
        case Instruction::SDiv:
        case Instruction::UDiv:
        case Instruction::SRem:
        case Instruction::URem:
          ++IntegerOps;
          break;

        case Instruction::FAdd:
        case Instruction::FSub:
        case Instruction::FMul:
        case Instruction::FDiv:
        case Instruction::FRem:
          ++FloatOps;
          break;

        default:
          break;
        }
      }
    }

    uint64_t TC = SE.getSmallConstantTripCount(L);

    TripKnown = TC != 0;
    TripCount = TC;

    unsigned MemoryOps = Loads + Stores;

    MemoryRatio =
        Instructions > 0 ? static_cast<double>(MemoryOps) / Instructions : 0.0;
  };

  collect(L1, F.loop1TripCountKnown, F.loop1TripCount, F.loop1NumInstructions,
          F.loop1NumLoads, F.loop1NumStores, F.loop1NumForwardContiguousLoads,
          F.loop1NumForwardContiguousStores, F.loop1NumIntegerOps,
          F.loop1NumFloatOps, F.loop1NumBranches, F.loop1NumCalls,
          F.loop1MemoryOpRatio, F.loop1ContiguousMemoryOps);

  collect(L2, F.loop2TripCountKnown, F.loop2TripCount, F.loop2NumInstructions,
          F.loop2NumLoads, F.loop2NumStores, F.loop2NumForwardContiguousLoads,
          F.loop2NumForwardContiguousStores, F.loop2NumIntegerOps,
          F.loop2NumFloatOps, F.loop2NumBranches, F.loop2NumCalls,
          F.loop2MemoryOpRatio, F.loop2ContiguousMemoryOps);

  F.loopsAdjacent = true;

  F.tripCountEqual = F.loop1TripCountKnown && F.loop2TripCountKnown &&
                     F.loop1TripCount == F.loop2TripCount;

  if (F.loop1TripCountKnown && F.loop2TripCountKnown && F.loop2TripCount != 0) {

    F.tripCountRatio = static_cast<double>(F.loop1TripCount) /
                       static_cast<double>(F.loop2TripCount);
  }

  F.combinedMemoryOps =
      F.loop1NumLoads + F.loop1NumStores + F.loop2NumLoads + F.loop2NumStores;

  return F;
}