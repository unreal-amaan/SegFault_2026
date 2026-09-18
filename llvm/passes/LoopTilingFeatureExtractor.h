#pragma once

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"

#include <cstdint>

namespace llvm {

struct LoopTilingFeatures {
  unsigned loopID = 0;

  // Loop structure
  unsigned loopDepth = 0;
  bool isInnermost = false;
  bool hasParentLoop = false;
  unsigned numSubLoops = 0;
  unsigned numBasicBlocks = 0;
  unsigned numExitingBlocks = 0;

  // Loop body
  unsigned numInstructions = 0;
  unsigned numPhiNodes = 0;
  unsigned numTerminatorInstructions = 0;

  // Memory
  unsigned numLoads = 0;
  unsigned numStores = 0;

  unsigned numForwardContiguousLoads = 0;
  unsigned numReverseContiguousLoads = 0;
  unsigned numStridedLoads = 0;
  unsigned numUnknownLoads = 0;

  unsigned numForwardContiguousStores = 0;
  unsigned numReverseContiguousStores = 0;
  unsigned numStridedStores = 0;
  unsigned numUnknownStores = 0;

  // Instruction mix
  unsigned numIntegerOps = 0;
  unsigned numFloatOps = 0;
  unsigned numBranches = 0;
  unsigned numConditionalBranches = 0;
  unsigned numCalls = 0;

  // Trip count
  bool tripCountKnown = false;
  uint64_t tripCount = 0;

  bool maxTripCountKnown = false;
  uint64_t maxTripCount = 0;

  // Derived features
  double memoryOpRatio = 0.0;
  double controlOverheadRatio = 0.0;
  double arithmeticIntensity = 0.0;
};

LoopTilingFeatures extractLoopTilingFeatures(Loop *L, ScalarEvolution &SE,
                                             const DataLayout &DL);

} // namespace llvm