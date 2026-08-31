#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Plugins/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

class FeatureExtractorPass : public PassInfoMixin<FeatureExtractorPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {

    errs() << "Function: " << F.getName() << "\n";

    return PreservedAnalyses::all();
  }
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "FeatureExtractor", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "feature-extractor") {
                    FPM.addPass(FeatureExtractorPass());
                    return true;
                  }

                  return false;
                });
          }};
}