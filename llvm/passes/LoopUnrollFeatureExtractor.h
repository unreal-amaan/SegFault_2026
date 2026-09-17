#pragma once

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"

#include <cstdint>

namespace llvm {

struct LoopUnrollFeatures {
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

  // Instruction mix
  unsigned numLoads = 0;
  unsigned numStores = 0;
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

LoopUnrollFeatures extractLoopUnrollFeatures(Loop *L, ScalarEvolution &SE);

} // namespace llvm
