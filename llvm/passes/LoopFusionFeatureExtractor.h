#pragma once

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"

#include <cstdint>

namespace llvm {

struct LoopFusionFeatures {
  unsigned loop1ID = 0;
  unsigned loop2ID = 0;

  bool loop1TripCountKnown = false;
  uint64_t loop1TripCount = 0;

  unsigned loop1NumInstructions = 0;
  unsigned loop1NumLoads = 0;
  unsigned loop1NumStores = 0;
  unsigned loop1NumForwardContiguousLoads = 0;
  unsigned loop1NumForwardContiguousStores = 0;
  unsigned loop1NumIntegerOps = 0;
  unsigned loop1NumFloatOps = 0;
  unsigned loop1NumBranches = 0;
  unsigned loop1NumCalls = 0;
  double loop1MemoryOpRatio = 0.0;
  unsigned loop1ContiguousMemoryOps = 0;

  bool loop2TripCountKnown = false;
  uint64_t loop2TripCount = 0;

  unsigned loop2NumInstructions = 0;
  unsigned loop2NumLoads = 0;
  unsigned loop2NumStores = 0;
  unsigned loop2NumForwardContiguousLoads = 0;
  unsigned loop2NumForwardContiguousStores = 0;
  unsigned loop2NumIntegerOps = 0;
  unsigned loop2NumFloatOps = 0;
  unsigned loop2NumBranches = 0;
  unsigned loop2NumCalls = 0;
  double loop2MemoryOpRatio = 0.0;
  unsigned loop2ContiguousMemoryOps = 0;

  bool loopsAdjacent = false;
  bool tripCountEqual = false;
  double tripCountRatio = 0.0;

  unsigned sharedMemoryObjects = 0;
  unsigned loop1StoresReadByLoop2 = 0;
  unsigned loop2StoresReadByLoop1 = 0;
  unsigned combinedMemoryOps = 0;
};

LoopFusionFeatures extractLoopFusionFeatures(Loop *L1, Loop *L2,
                                             ScalarEvolution &SE);
} // namespace llvm