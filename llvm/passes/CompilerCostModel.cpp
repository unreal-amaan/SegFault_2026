#include "LoopUnrollFeatureExtractor.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Plugins/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdint>
#include <limits>
#include <map>
#include <string>

using namespace llvm;

namespace {

enum class OptimizationKind { Unroll, Vectorize, Fusion, Tiling };

struct OptimizationRequest {
  OptimizationKind Kind;
  std::map<std::string, int64_t> Parameters;
};

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

static void collectLoops(Loop *L, SmallVectorImpl<Loop *> &Loops) {
  Loops.push_back(L);

  for (Loop *SubLoop : L->getSubLoops())
    collectLoops(SubLoop, Loops);
}

class CompilerCostModelPass : public PassInfoMixin<CompilerCostModelPass> {

public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {

    errs() << "[CostModel] Function: " << F.getName() << "\n";

    LoopInfo &LI = FAM.getResult<LoopAnalysis>(F);

    // Assign every instruction a position in the function.
    //
    // This lets us determine which loop appears after an
    // annotation call.

    std::map<const Instruction *, unsigned> InstructionOrder;

    unsigned Index = 0;

    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        InstructionOrder[&I] = Index++;
      }
    }

    // Collect all loops, including nested loops.

    SmallVector<Loop *, 16> AllLoops;

    for (Loop *TopLevelLoop : LI)
      collectLoops(TopLevelLoop, AllLoops);

    // Search for cost-model annotation calls.

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

        // ------------------------------------------------------------
        // Unroll
        // ------------------------------------------------------------

        if (Name == "__costmodel_unroll") {

          Request.Kind = OptimizationKind::Unroll;

          if (Call->arg_size() >= 1) {

            if (auto *Constant =
                    dyn_cast<ConstantInt>(Call->getArgOperand(0))) {

              Request.Parameters["factor"] = Constant->getSExtValue();
            }
          }
        }

        // ------------------------------------------------------------
        // Vectorization
        // ------------------------------------------------------------

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

        // ------------------------------------------------------------
        // Tiling
        // ------------------------------------------------------------

        else if (Name == "__costmodel_tiling") {

          Request.Kind = OptimizationKind::Tiling;

          if (Call->arg_size() >= 1) {

            if (auto *Constant =
                    dyn_cast<ConstantInt>(Call->getArgOperand(0))) {

              Request.Parameters["tile_size"] = Constant->getSExtValue();
            }
          }
        }

        // ------------------------------------------------------------
        // Fusion
        // ------------------------------------------------------------

        else if (Name == "__costmodel_fusion") {

          Request.Kind = OptimizationKind::Fusion;
        }

        
        // Not one of our annotation functions.

        else {
          continue;
        }

        // ------------------------------------------------------------
        // Print annotation
        // ------------------------------------------------------------

        errs() << "  [CostModel] Annotation: " << optimizationName(Request.Kind)
               << "\n";

        for (const auto &[ParamName, Value] : Request.Parameters) {

          errs() << "      " << ParamName << " = " << Value << "\n";
        }

        // ------------------------------------------------------------
        // Find the next loop
        // ------------------------------------------------------------

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

        // ------------------------------------------------------------
        // No loop after annotation
        // ------------------------------------------------------------

        if (!NextLoop) {

          errs() << "  [CostModel] "
                    "No following loop - skipping\n";

          continue;
        }

        // ------------------------------------------------------------
        // Successfully associated annotation with loop
        // ------------------------------------------------------------

        errs() << "  [CostModel] Associated loop: ";

        if (NextLoop->getHeader()->hasName()) {

          errs() << NextLoop->getHeader()->getName();

        } else {

          errs() << "<unnamed>";
        }

        errs() << "\n";
      }
    }

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