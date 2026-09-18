#include "LoopFusionFeatureExtractor.h"
#include "LoopTilingFeatureExtractor.h"
#include "LoopUnrollFeatureExtractor.h"
#include "LoopVectorizationFeatureExtractor.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Plugins/PassPlugin.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdint>
#include <limits>
#include <map>
#include <string>

using namespace llvm;

namespace {

// -----------------------------------------------------------------------------
// Optimization types
// -----------------------------------------------------------------------------

enum class OptimizationKind { Unroll, Vectorize, Fusion, Tiling };

// -----------------------------------------------------------------------------
// Cost-model record
// -----------------------------------------------------------------------------

struct CostModelRecord {
  OptimizationKind Kind;
  unsigned LoopID;

  std::string FunctionName;
  std::string LoopName;

  std::map<std::string, int64_t> Parameters;

  LoopUnrollFeatures UnrollFeatures;
  LoopVectorizationFeatures VectorizationFeatures;
  LoopFusionFeatures FusionFeatures;
  LoopTilingFeatures TilingFeatures;
};

// -----------------------------------------------------------------------------
// Optimization request
// -----------------------------------------------------------------------------

struct OptimizationRequest {
  OptimizationKind Kind;
  std::map<std::string, int64_t> Parameters;
};

// -----------------------------------------------------------------------------
// Global JSON storage
//
// The pass runs once per function. Therefore we keep all records in one
// process-wide JSON array and rewrite the JSON file after every function.
//
// This gives us one final JSON file containing records from all functions.
// -----------------------------------------------------------------------------

static json::Array AllRecords;

// Output file name. It is created relative to the directory from which
// opt is executed.
static constexpr const char *JSONOutputFile = "costmodel_features.json";

// -----------------------------------------------------------------------------
// Optimization name
// -----------------------------------------------------------------------------

static StringRef optimizationName(OptimizationKind Kind) {
  switch (Kind) {
  case OptimizationKind::Unroll:
    return "unroll";

  case OptimizationKind::Vectorize:
    return "vectorize";

  case OptimizationKind::Fusion:
    return "fusion";

  case OptimizationKind::Tiling:
    return "tiling";
  }

  llvm_unreachable("unknown optimization kind");
}

// -----------------------------------------------------------------------------
// Collect loops recursively
// -----------------------------------------------------------------------------

static void collectLoops(Loop *L, SmallVectorImpl<Loop *> &Loops) {
  Loops.push_back(L);

  for (Loop *SubLoop : L->getSubLoops())
    collectLoops(SubLoop, Loops);
}

// -----------------------------------------------------------------------------
// JSON helpers
// -----------------------------------------------------------------------------

static void addCommonFeatures(json::Object &Features,
                              const LoopUnrollFeatures &F) {
  Features["loop_depth"] = F.loopDepth;
  Features["innermost"] = F.isInnermost;
  Features["has_parent_loop"] = F.hasParentLoop;
  Features["num_subloops"] = F.numSubLoops;
  Features["num_basic_blocks"] = F.numBasicBlocks;
  Features["num_exiting_blocks"] = F.numExitingBlocks;
  Features["num_phi_nodes"] = F.numPhiNodes;
  Features["num_terminator_instructions"] = F.numTerminatorInstructions;

  Features["num_instructions"] = F.numInstructions;
  Features["num_loads"] = F.numLoads;
  Features["num_stores"] = F.numStores;
  Features["num_integer_ops"] = F.numIntegerOps;
  Features["num_float_ops"] = F.numFloatOps;
  Features["num_branches"] = F.numBranches;
  Features["num_conditional_branches"] = F.numConditionalBranches;
  Features["num_calls"] = F.numCalls;

  Features["trip_count_known"] = F.tripCountKnown;
  Features["trip_count"] = F.tripCount;
  Features["max_trip_count_known"] = F.maxTripCountKnown;
  Features["max_trip_count"] = F.maxTripCount;

  Features["memory_op_ratio"] = F.memoryOpRatio;
  Features["control_overhead_ratio"] = F.controlOverheadRatio;
  Features["arithmetic_intensity"] = F.arithmeticIntensity;
}

static void addCommonFeatures(json::Object &Features,
                              const LoopVectorizationFeatures &F) {
  Features["loop_depth"] = F.loopDepth;
  Features["innermost"] = F.isInnermost;
  Features["has_parent_loop"] = F.hasParentLoop;
  Features["num_subloops"] = F.numSubLoops;
  Features["num_basic_blocks"] = F.numBasicBlocks;
  Features["num_exiting_blocks"] = F.numExitingBlocks;
  Features["num_phi_nodes"] = F.numPhiNodes;
  Features["num_terminator_instructions"] = F.numTerminatorInstructions;

  Features["num_instructions"] = F.numInstructions;
  Features["num_loads"] = F.numLoads;
  Features["num_stores"] = F.numStores;
  Features["num_integer_ops"] = F.numIntegerOps;
  Features["num_float_ops"] = F.numFloatOps;
  Features["num_branches"] = F.numBranches;
  Features["num_conditional_branches"] = F.numConditionalBranches;
  Features["num_calls"] = F.numCalls;

  Features["trip_count_known"] = F.tripCountKnown;
  Features["trip_count"] = F.tripCount;
  Features["max_trip_count_known"] = F.maxTripCountKnown;
  Features["max_trip_count"] = F.maxTripCount;

  Features["memory_op_ratio"] = F.memoryOpRatio;
  Features["control_overhead_ratio"] = F.controlOverheadRatio;
  Features["arithmetic_intensity"] = F.arithmeticIntensity;

  Features["contiguous_loads"] =
      F.numForwardContiguousLoads + F.numReverseContiguousLoads;

  Features["strided_loads"] = F.numStridedLoads;
  Features["unknown_loads"] = F.numUnknownLoads;

  Features["contiguous_stores"] =
      F.numForwardContiguousStores + F.numReverseContiguousStores;

  Features["strided_stores"] = F.numStridedStores;
  Features["unknown_stores"] = F.numUnknownStores;

  Features["has_reduction"] = F.hasReduction;
  Features["num_reductions"] = F.numReductions;
}

static void addCommonFeatures(json::Object &Features,
                              const LoopFusionFeatures &F) {

  Features["loop1_trip_count_known"] = F.loop1TripCountKnown;
  Features["loop1_trip_count"] = F.loop1TripCount;
  Features["loop1_num_instructions"] = F.loop1NumInstructions;
  Features["loop1_num_loads"] = F.loop1NumLoads;
  Features["loop1_num_stores"] = F.loop1NumStores;
  Features["loop1_num_forward_contiguous_loads"] =
      F.loop1NumForwardContiguousLoads;
  Features["loop1_num_forward_contiguous_stores"] =
      F.loop1NumForwardContiguousStores;
  Features["loop1_num_integer_ops"] = F.loop1NumIntegerOps;
  Features["loop1_num_float_ops"] = F.loop1NumFloatOps;
  Features["loop1_num_branches"] = F.loop1NumBranches;
  Features["loop1_num_calls"] = F.loop1NumCalls;
  Features["loop1_memory_op_ratio"] = F.loop1MemoryOpRatio;
  Features["loop1_contiguous_memory_ops"] = F.loop1ContiguousMemoryOps;

  Features["loop2_trip_count_known"] = F.loop2TripCountKnown;
  Features["loop2_trip_count"] = F.loop2TripCount;
  Features["loop2_num_instructions"] = F.loop2NumInstructions;
  Features["loop2_num_loads"] = F.loop2NumLoads;
  Features["loop2_num_stores"] = F.loop2NumStores;
  Features["loop2_num_forward_contiguous_loads"] =
      F.loop2NumForwardContiguousLoads;
  Features["loop2_num_forward_contiguous_stores"] =
      F.loop2NumForwardContiguousStores;
  Features["loop2_num_integer_ops"] = F.loop2NumIntegerOps;
  Features["loop2_num_float_ops"] = F.loop2NumFloatOps;
  Features["loop2_num_branches"] = F.loop2NumBranches;
  Features["loop2_num_calls"] = F.loop2NumCalls;
  Features["loop2_memory_op_ratio"] = F.loop2MemoryOpRatio;
  Features["loop2_contiguous_memory_ops"] = F.loop2ContiguousMemoryOps;

  Features["loops_adjacent"] = F.loopsAdjacent;
  Features["trip_count_equal"] = F.tripCountEqual;
  Features["trip_count_ratio"] = F.tripCountRatio;

  Features["shared_memory_objects"] = F.sharedMemoryObjects;
  Features["loop1_stores_read_by_loop2"] = F.loop1StoresReadByLoop2;
  Features["loop2_stores_read_by_loop1"] = F.loop2StoresReadByLoop1;

  Features["combined_memory_ops"] = F.combinedMemoryOps;
}

static void addCommonFeatures(json::Object &Features,
                              const LoopTilingFeatures &F) {
  Features["loop_depth"] = F.loopDepth;
  Features["innermost"] = F.isInnermost;
  Features["has_parent_loop"] = F.hasParentLoop;
  Features["num_subloops"] = F.numSubLoops;
  Features["num_basic_blocks"] = F.numBasicBlocks;
  Features["num_exiting_blocks"] = F.numExitingBlocks;
  Features["num_phi_nodes"] = F.numPhiNodes;
  Features["num_terminator_instructions"] = F.numTerminatorInstructions;

  Features["num_instructions"] = F.numInstructions;
  Features["num_loads"] = F.numLoads;
  Features["num_stores"] = F.numStores;
  Features["num_integer_ops"] = F.numIntegerOps;
  Features["num_float_ops"] = F.numFloatOps;
  Features["num_branches"] = F.numBranches;
  Features["num_conditional_branches"] = F.numConditionalBranches;
  Features["num_calls"] = F.numCalls;

  Features["trip_count_known"] = F.tripCountKnown;
  Features["trip_count"] = F.tripCount;
  Features["max_trip_count_known"] = F.maxTripCountKnown;
  Features["max_trip_count"] = F.maxTripCount;

  Features["memory_op_ratio"] = F.memoryOpRatio;
  Features["control_overhead_ratio"] = F.controlOverheadRatio;
  Features["arithmetic_intensity"] = F.arithmeticIntensity;
}

// -----------------------------------------------------------------------------
// Convert a CostModelRecord to JSON
// -----------------------------------------------------------------------------

static json::Object recordToJSON(const CostModelRecord &Record) {

  json::Object Object;

  Object["function"] = Record.FunctionName;
  Object["optimization"] = optimizationName(Record.Kind);
  Object["loop_id"] = Record.LoopID;
  Object["loop_name"] = Record.LoopName;

  // Optimization parameters.
  json::Object Parameters;

  for (const auto &[Name, Value] : Record.Parameters)
    Parameters[Name] = Value;

  Object["parameters"] = std::move(Parameters);

  // Optimization-specific features.
  json::Object Features;

  switch (Record.Kind) {

  case OptimizationKind::Unroll:
    addCommonFeatures(Features, Record.UnrollFeatures);
    break;

  case OptimizationKind::Vectorize:
    addCommonFeatures(Features, Record.VectorizationFeatures);
    break;

  case OptimizationKind::Fusion:
    addCommonFeatures(Features, Record.FusionFeatures);
    break;

  case OptimizationKind::Tiling:
    addCommonFeatures(Features, Record.TilingFeatures);
    break;
  }

  Object["features"] = std::move(Features);

  return Object;
}

// -----------------------------------------------------------------------------
// Write complete JSON file
// -----------------------------------------------------------------------------

static void writeJSONFile() {

  std::error_code EC;

  raw_fd_ostream OS(JSONOutputFile, EC);

  if (EC) {
    errs() << "[CostModel] ERROR: Could not open JSON output file '"
           << JSONOutputFile << "': " << EC.message() << "\n";
    return;
  }

  json::Value Root(std::move(AllRecords));
  OS << formatv("{0:2}\n", Root);

  errs() << "[CostModel] JSON written to " << JSONOutputFile << "\n";
}

// -----------------------------------------------------------------------------
// Compiler Cost Model Pass
// -----------------------------------------------------------------------------

class CompilerCostModelPass : public PassInfoMixin<CompilerCostModelPass> {

public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {

    errs() << "[CostModel] Function: " << F.getName() << "\n";

    LoopInfo &LI = FAM.getResult<LoopAnalysis>(F);
    ScalarEvolution &SE = FAM.getResult<ScalarEvolutionAnalysis>(F);

    // -------------------------------------------------------------------------
    // Assign every instruction a position in the function.
    //
    // This lets us determine which loop appears after an annotation call.
    // -------------------------------------------------------------------------

    std::map<const Instruction *, unsigned> InstructionOrder;

    unsigned Index = 0;

    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        InstructionOrder[&I] = Index++;
      }
    }

    // -------------------------------------------------------------------------
    // Collect all loops, including nested loops.
    // -------------------------------------------------------------------------

    SmallVector<Loop *, 16> AllLoops;

    for (Loop *TopLevelLoop : LI)
      collectLoops(TopLevelLoop, AllLoops);

    // -------------------------------------------------------------------------
    // Search for cost-model annotation calls.
    // -------------------------------------------------------------------------

    for (BasicBlock &BB : F) {

      for (Instruction &I : BB) {

        auto *Call = dyn_cast<CallBase>(&I);

        if (!Call)
          continue;

        Function *CalledFunction = Call->getCalledFunction();

        if (!CalledFunction)
          continue;

        StringRef Name = CalledFunction->getName();

        OptimizationRequest Request;

        // ---------------------------------------------------------------------
        // Unroll
        // ---------------------------------------------------------------------

        if (Name == "__costmodel_unroll") {

          Request.Kind = OptimizationKind::Unroll;

          if (Call->arg_size() >= 1) {

            if (auto *Constant =
                    dyn_cast<ConstantInt>(Call->getArgOperand(0))) {

              Request.Parameters["factor"] = Constant->getSExtValue();
            }
          }
        }

        // ---------------------------------------------------------------------
        // Vectorization
        // ---------------------------------------------------------------------

        else if (Name == "__costmodel_vectorize") {

          Request.Kind = OptimizationKind::Vectorize;

          if (Call->arg_size() >= 1) {

            if (auto *Constant =
                    dyn_cast<ConstantInt>(Call->getArgOperand(0))) {

              Request.Parameters["vf"] = Constant->getSExtValue();
            }
          }

          if (Call->arg_size() >= 2) {

            if (auto *Constant =
                    dyn_cast<ConstantInt>(Call->getArgOperand(1))) {

              Request.Parameters["if"] = Constant->getSExtValue();
            }
          }
        }

        // ---------------------------------------------------------------------
        // Tiling
        // ---------------------------------------------------------------------

        else if (Name == "__costmodel_tiling") {

          Request.Kind = OptimizationKind::Tiling;

          if (Call->arg_size() >= 1) {

            if (auto *Constant =
                    dyn_cast<ConstantInt>(Call->getArgOperand(0))) {

              Request.Parameters["tile_size"] = Constant->getSExtValue();
            }
          }
        }

        // ---------------------------------------------------------------------
        // Fusion
        // ---------------------------------------------------------------------

        else if (Name == "__costmodel_fusion") {

          Request.Kind = OptimizationKind::Fusion;
        }

        // Not one of our annotation functions.
        else {
          continue;
        }

        // ---------------------------------------------------------------------
        // Print annotation
        // ---------------------------------------------------------------------

        errs() << "  [CostModel] Annotation: " << optimizationName(Request.Kind)
               << "\n";

        for (const auto &[ParamName, Value] : Request.Parameters) {
          errs() << "      " << ParamName << " = " << Value << "\n";
        }

        // ---------------------------------------------------------------------
        // Find the next loop.
        // ---------------------------------------------------------------------

        auto AnnotationIt = InstructionOrder.find(&I);

        if (AnnotationIt == InstructionOrder.end())
          continue;

        unsigned AnnotationPosition = AnnotationIt->second;

        Loop *NextLoop = nullptr;

        unsigned BestPosition = std::numeric_limits<unsigned>::max();

        for (Loop *L : AllLoops) {

          BasicBlock *Header = L->getHeader();

          if (!Header || Header->empty())
            continue;

          Instruction *HeaderInstruction = &Header->front();

          auto LoopIt = InstructionOrder.find(HeaderInstruction);

          if (LoopIt == InstructionOrder.end())
            continue;

          unsigned LoopPosition = LoopIt->second;

          // The loop must occur AFTER the annotation.
          if (LoopPosition <= AnnotationPosition)
            continue;

          // Select the closest following loop.
          if (LoopPosition < BestPosition) {

            BestPosition = LoopPosition;
            NextLoop = L;
          }
        }

        // ---------------------------------------------------------------------
        // No loop after annotation.
        // ---------------------------------------------------------------------

        if (!NextLoop) {

          errs() << "  [CostModel] "
                    "No following loop - skipping\n";

          continue;
        }

        // ---------------------------------------------------------------------
        // Successfully associated annotation with loop.
        // ---------------------------------------------------------------------

        errs() << "  [CostModel] Associated loop: ";

        std::string LoopName;

        if (NextLoop->getHeader()->hasName()) {

          LoopName = NextLoop->getHeader()->getName().str();

          errs() << LoopName;

        } else {

          LoopName = "<unnamed>";

          errs() << LoopName;
        }

        errs() << "\n";

        // ---------------------------------------------------------------------
        // Create record.
        // ---------------------------------------------------------------------

        CostModelRecord Record{};

        Record.Kind = Request.Kind;
        Record.Parameters = Request.Parameters;
        Record.FunctionName = F.getName().str();
        Record.LoopName = LoopName;

        // ---------------------------------------------------------------------
        // Dispatch to optimization-specific feature extractor.
        // ---------------------------------------------------------------------

        switch (Request.Kind) {

        case OptimizationKind::Unroll: {

          Record.UnrollFeatures = extractLoopUnrollFeatures(NextLoop, SE);

          Record.LoopID = Record.UnrollFeatures.loopID;

          errs() << "  [CostModel] Unroll features extracted\n";

          break;
        }

        case OptimizationKind::Vectorize: {

          Record.VectorizationFeatures =
              extractLoopVectorizationFeatures(NextLoop, SE, F.getDataLayout());

          Record.LoopID = Record.VectorizationFeatures.loopID;

          errs() << "  [CostModel] Vectorization features extracted\n";

          break;
        }

        case OptimizationKind::Tiling: {

          Record.TilingFeatures =
              extractLoopTilingFeatures(NextLoop, SE, F.getDataLayout());

          Record.LoopID = Record.TilingFeatures.loopID;

          errs() << "  [CostModel] Tiling features extracted\n";

          break;
        }

        case OptimizationKind::Fusion: {

          Loop *Loop1 = NextLoop;
          Loop *Loop2 = nullptr;

          // Find the next loop after Loop1.
          unsigned Loop1Position =
              InstructionOrder[&Loop1->getHeader()->front()];

          unsigned BestLoop2Position = std::numeric_limits<unsigned>::max();

          for (Loop *Candidate : AllLoops) {

            if (Candidate == Loop1)
              continue;

            BasicBlock *Header = Candidate->getHeader();

            if (!Header || Header->empty())
              continue;

            auto It = InstructionOrder.find(&Header->front());

            if (It == InstructionOrder.end())
              continue;

            if (It->second <= Loop1Position)
              continue;

            if (It->second < BestLoop2Position) {
              BestLoop2Position = It->second;
              Loop2 = Candidate;
            }
          }

          if (!Loop2) {
            errs() << "  [CostModel] Fusion requires two loops - skipping\n";
            continue;
          }

          Record.FusionFeatures = extractLoopFusionFeatures(Loop1, Loop2, SE);

          Record.LoopID = Record.FusionFeatures.loop1ID;

          Record.LoopName = Loop1->getHeader()->hasName()
                                ? Loop1->getHeader()->getName().str()
                                : "<unnamed>";

          errs() << "  [CostModel] Fusion pair: " << Record.LoopName << " + ";

          if (Loop2->getHeader()->hasName())
            errs() << Loop2->getHeader()->getName();

          else
            errs() << "<unnamed>";

          errs() << "\n";

          Record.LoopID = Record.FusionFeatures.loop1ID;

          errs() << "  [CostModel] Fusion features extracted\n";

          break;
        }
        }

        // ---------------------------------------------------------------------
        // Print features to terminal.
        // ---------------------------------------------------------------------

        errs() << "  [CostModel] Record created"
               << " | optimization=" << optimizationName(Record.Kind)
               << " | loop_id=" << Record.LoopID << "\n";

        // ---------------------------------------------------------------------
        // Add record to global JSON array.
        // ---------------------------------------------------------------------

        AllRecords.push_back(recordToJSON(Record));

        errs() << "  [CostModel] Added record to JSON\n";
      }
    }

    // -------------------------------------------------------------------------
    // Write all records collected so far.
    //
    // Since this is a FunctionPass, this happens once for every function.
    // The global array ensures previous function records are retained.
    // -------------------------------------------------------------------------

    if (!AllRecords.empty())
      writeJSONFile();

    return PreservedAnalyses::all();
  }
};

} // namespace

// -----------------------------------------------------------------------------
// LLVM pass plugin registration
// -----------------------------------------------------------------------------

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {

  return {LLVM_PLUGIN_API_VERSION, "CompilerCostModel", LLVM_VERSION_STRING,

          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "compiler-cost-model") {

                    FPM.addPass(CompilerCostModelPass());

                    return true;
                  }

                  return false;
                });
          }};
}
