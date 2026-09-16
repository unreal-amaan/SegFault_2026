#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Plugins/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include <optional>

using namespace llvm;


static cl::opt<unsigned> TargetLoopID(
    "target-loop-id",
    cl::desc("Loop ID to apply unrolling metadata to"),
    cl::init(0));

static cl::opt<unsigned> UnrollFactor(
    "unroll-factor",
    cl::desc("Loop unroll factor"),
    cl::init(2));





class LoopUnrollDatasetGeneratorPass
    : public PassInfoMixin<LoopUnrollDatasetGeneratorPass> {

public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {

    if (F.isDeclaration())
      return PreservedAnalyses::all();

    LoopInfo &LI = AM.getResult<LoopAnalysis>(F);

    errs() << "\nFunction: " << F.getName() << "\n";

    for (Loop *L : LI) {
      findLoop(L);
    }

    return PreservedAnalyses::none();
  }

private:
  void findLoop(Loop *L) {

    if (auto LoopID = getLoopID(L)) {
      errs() << "Found loop ID: " << *LoopID << "\n";

      BasicBlock *Header = L->getHeader();

      errs() << "Header block: " << Header->getName() << "\n";
      if (*LoopID == TargetLoopID) {
        errs() << "Selected loop ID: " << *LoopID << "\n";
        addUnrollMetadata(L);
      }
    }

    for (Loop *SubLoop : *L) {
      findLoop(SubLoop);
    }
  }

  std::optional<unsigned> getLoopID(Loop *L) {

    MDNode *LoopMetadata = L->getLoopID();

    if (!LoopMetadata)
      return std::nullopt;

    for (unsigned i = 1; i < LoopMetadata->getNumOperands(); ++i) {

      MDNode *Metadata = dyn_cast<MDNode>(LoopMetadata->getOperand(i));

      if (!Metadata || Metadata->getNumOperands() < 2)
        continue;

      auto *Name = dyn_cast<MDString>(Metadata->getOperand(0));

      if (!Name)
        continue;

      if (Name->getString() != "compiler_cost_model.loop_id")
        continue;

      auto *IDValue =
          mdconst::dyn_extract<ConstantInt>(Metadata->getOperand(1));

      if (!IDValue)
        continue;

      return IDValue->getZExtValue();
    }

    return std::nullopt;
  }

  void addUnrollMetadata(Loop *L) {
    LLVMContext &Ctx = L->getHeader()->getContext();

    MDNode *OldLoopID = L->getLoopID();

    SmallVector<Metadata *, 8> MDs;

    // First operand of loop metadata must refer to itself.
    MDs.push_back(nullptr);

    // Preserve existing loop metadata.
    if (OldLoopID) {
      for (unsigned i = 1; i < OldLoopID->getNumOperands(); ++i) {
        MDs.push_back(OldLoopID->getOperand(i));
      }
    }

    // Add: llvm.loop.unroll.count = UnrollFactor
    Metadata *UnrollMD[] = {MDString::get(Ctx, "llvm.loop.unroll.count"),
                            ConstantAsMetadata::get(ConstantInt::get(
                                Type::getInt32Ty(Ctx), UnrollFactor))};

    MDs.push_back(MDNode::get(Ctx, UnrollMD));

    MDNode *NewLoopID = MDNode::getDistinct(Ctx, MDs);

    // Make the first operand reference itself.
    NewLoopID->replaceOperandWith(0, NewLoopID);

    L->setLoopID(NewLoopID);

    errs() << "Attached unroll factor " << UnrollFactor
           << " to selected loop\n";
  }
};



extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {

  return {LLVM_PLUGIN_API_VERSION, "LoopUnrollDatasetGenerator",
          LLVM_VERSION_STRING,

          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "loop-unroll-dataset-generator") {

                    FPM.addPass(LoopUnrollDatasetGeneratorPass());

                    return true;
                  }

                  return false;
                });
          }};
}