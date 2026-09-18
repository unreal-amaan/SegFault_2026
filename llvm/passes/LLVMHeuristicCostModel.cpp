#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Plugins/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

    using namespace llvm;

namespace {

struct Annotation {
  std::string Optimization;
  Instruction *Call;
};

struct LLVMHeuristicCostModelPass
    : public PassInfoMixin<LLVMHeuristicCostModelPass> {

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
    auto &LI = FAM.getResult<LoopAnalysis>(F);
    auto &TTI = FAM.getResult<TargetIRAnalysis>(F);

    errs() << "=== LLVM HEURISTIC COST MODEL ===\n";
    errs() << "Function: " << F.getName() << "\n";

    std::vector<Annotation> Annotations;

    // ------------------------------------------------------------
    // Find all cost-model annotations in the function.
    // ------------------------------------------------------------

    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        auto *Call = dyn_cast<CallInst>(&I);
        if (!Call)
          continue;

        Function *Callee = Call->getCalledFunction();
        if (!Callee)
          continue;

        StringRef Name = Callee->getName();

        if (Name == "__costmodel_unroll") {
          Annotations.push_back({"unroll", &I});
        } else if (Name == "__costmodel_vectorize") {
          Annotations.push_back({"vectorize", &I});
        } else if (Name == "__costmodel_tiling") {
          Annotations.push_back({"tiling", &I});
        } else if (Name == "__costmodel_fusion") {
          Annotations.push_back({"fusion", &I});
        }
      }
    }

    if (Annotations.empty()) {
      return PreservedAnalyses::all();
    }

    // ------------------------------------------------------------
    // Collect all loops recursively.
    // ------------------------------------------------------------

    std::vector<Loop *> Loops;

    std::function<void(Loop *)> CollectLoops = [&](Loop *L) {
      Loops.push_back(L);

      for (Loop *SubLoop : L->getSubLoops())
        CollectLoops(SubLoop);
    };

    for (Loop *L : LI)
      CollectLoops(L);

    // ------------------------------------------------------------
    // Sort loops according to their header position in the
    // function's instruction order.
    // ------------------------------------------------------------

    std::map<Instruction *, unsigned> InstructionOrder;

    unsigned Order = 0;

    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        InstructionOrder[&I] = Order++;
      }
    }

    std::sort(Loops.begin(), Loops.end(), [&](Loop *A, Loop *B) {
      Instruction *AHeader = &*A->getHeader()->begin();

      Instruction *BHeader = &*B->getHeader()->begin();

      return InstructionOrder[AHeader] < InstructionOrder[BHeader];
    });

    // ------------------------------------------------------------
    // Associate each annotation with the next loop.
    //
    // This handles:
    //
    //   __costmodel_tiling(32);
    //   __costmodel_unroll(4);
    //   __costmodel_fusion();
    //   for (...) {
    //
    // All three annotations refer to that loop.
    //
    // It also handles annotations immediately before nested loops.
    // ------------------------------------------------------------

    for (const Annotation &A : Annotations) {

      unsigned AnnotationOrder = InstructionOrder[A.Call];

      Loop *TargetLoop = nullptr;

      unsigned BestOrder = UINT_MAX;

      for (Loop *L : Loops) {

        Instruction *Header = &*L->getHeader()->begin();

        unsigned HeaderOrder = InstructionOrder[Header];

        if (HeaderOrder > AnnotationOrder && HeaderOrder < BestOrder) {

          BestOrder = HeaderOrder;
          TargetLoop = L;
        }
      }

      if (!TargetLoop)
        continue;

      // ----------------------------------------------------------
      // Avoid evaluating the same optimization/loop pair twice.
      // ----------------------------------------------------------

      analyzeLoop(TargetLoop, TTI, A.Optimization);
    }

    return PreservedAnalyses::all();
  }

  void analyzeLoop(Loop *L, TargetTransformInfo &TTI, StringRef Optimization) {

    int64_t Cost = 0;

    unsigned Instructions = 0;
    unsigned MemoryOps = 0;
    unsigned Branches = 0;

    // ------------------------------------------------------------
    // Collect loop features.
    // ------------------------------------------------------------

    for (BasicBlock *BB : L->blocks()) {

      for (Instruction &I : *BB) {

        if (I.isTerminator()) {
          Branches++;
          continue;
        }

        Instructions++;

        if (I.mayReadOrWriteMemory())
          MemoryOps++;

        // LLVM target-specific instruction cost.
        InstructionCost IC = TTI.getInstructionCost(
            &I, TargetTransformInfo::TCK_RecipThroughput);

        if (IC.isValid())
          Cost += IC.getValue();
        else
          Cost += 1;
      }
    }

    double MemoryRatio =
        Instructions == 0 ? 0.0 : static_cast<double>(MemoryOps) / Instructions;

    double BranchRatio =
        Instructions == 0 ? 0.0 : static_cast<double>(Branches) / Instructions;

    // ------------------------------------------------------------
    // LLVM-based heuristic
    // ------------------------------------------------------------

    double Score = 0.0;

    // Large loops are generally better optimization candidates.
    if (Instructions >= 30)
      Score += 0.30;
    else if (Instructions >= 15)
      Score += 0.15;

    // Memory-heavy loops can benefit from transformations
    // such as vectorization and tiling.
    if (MemoryRatio >= 0.50)
      Score += 0.30;
    else if (MemoryRatio >= 0.30)
      Score += 0.15;

    // Avoid loops dominated by control flow.
    if (BranchRatio < 0.10)
      Score += 0.25;
    else if (BranchRatio < 0.20)
      Score += 0.15;

    // LLVM target-specific cost.
    if (Cost >= 30)
      Score += 0.15;
    else if (Cost >= 15)
      Score += 0.10;

    bool Profitable = Score >= 0.50;

    // ------------------------------------------------------------
    // Output
    // ------------------------------------------------------------

    BasicBlock *Header = L->getHeader();

    std::string LoopName;

    if (Header)
      LoopName = Header->getName().str();

    errs() << "\n";
    errs() << "Optimization : " << Optimization << "\n";
    errs() << "Loop         : " << LoopName << "\n";
    errs() << "  instructions : " << Instructions << "\n";
    errs() << "  memory ops   : " << MemoryOps << "\n";
    errs() << "  branches     : " << Branches << "\n";
    errs() << "  LLVM cost    : " << Cost << "\n";
    errs() << "  memory ratio : " << MemoryRatio << "\n";
    errs() << "  branch ratio : " << BranchRatio << "\n";
    errs() << "  score        : " << Score << "\n";
    errs() << "  profitable   : " << (Profitable ? "YES" : "NO") << "\n";
  }
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {

  return {LLVM_PLUGIN_API_VERSION, "LLVMHeuristicCostModel",
          LLVM_VERSION_STRING,

          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "llvm-heuristic-cost-model") {

                    FPM.addPass(LLVMHeuristicCostModelPass());

                    return true;
                  }

                  return false;
                });
          }};
}
