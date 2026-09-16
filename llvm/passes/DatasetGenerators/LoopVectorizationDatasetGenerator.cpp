#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Plugins/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include <optional>

using namespace llvm;

static cl::opt<unsigned>
    TargetLoopID("target-loop-id",
                 cl::desc("Loop ID to apply vectorization metadata to"),
                 cl::init(0));

static cl::opt<unsigned> VectorizationFactor("vectorization-factor",
                                             cl::desc("Vectorization factor"),
                                             cl::init(4));

static cl::opt<unsigned> InterleaveFactor("interleave-factor",
                                          cl::desc("Interleave factor"),
                                          cl::init(1));

class LoopVectorizationDatasetGeneratorPass
    : public PassInfoMixin<LoopVectorizationDatasetGeneratorPass> {

public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {

    if (F.isDeclaration())
      return PreservedAnalyses::all();

    LoopInfo &LI = AM.getResult<LoopAnalysis>(F);

    errs() << "\nFunction: " << F.getName() << "\n";

    for (Loop *L : LI)
      findLoop(L);

    return PreservedAnalyses::none();
  }

private:
  void findLoop(Loop *L) {

    if (auto LoopID = getLoopID(L)) {
      errs() << "Found loop ID: " << *LoopID << "\n";

      if (*LoopID == TargetLoopID) {
        errs() << "Selected loop ID: " << *LoopID << "\n";
        addVectorizationMetadata(L);
      } else {
        addVectorizationDisableMetadata(L);
      }
    }

    for (Loop *SubLoop : *L)
      findLoop(SubLoop);
  }

  void addVectorizationDisableMetadata(Loop *L) {
    LLVMContext &Ctx = L->getHeader()->getContext();
    MDNode *OldLoopID = L->getLoopID();

    SmallVector<Metadata *, 8> MDs;

    // First operand must refer to the loop metadata node itself.
    MDs.push_back(nullptr);

    // Preserve existing metadata.
    if (OldLoopID) {
      for (unsigned i = 1; i < OldLoopID->getNumOperands(); ++i) {
        MDs.push_back(OldLoopID->getOperand(i));
      }
    }

    Metadata *VectorizeEnableMD[] = {
        MDString::get(Ctx, "llvm.loop.vectorize.enable"),
        ConstantAsMetadata::get(ConstantInt::getFalse(Ctx))};

    MDs.push_back(MDNode::get(Ctx, VectorizeEnableMD));

    MDNode *NewLoopID = MDNode::getDistinct(Ctx, MDs);

    // Make first operand self-reference.
    NewLoopID->replaceOperandWith(0, NewLoopID);

    L->setLoopID(NewLoopID);

    errs() << "Disabled vectorization for non-target loop\n";
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

  void addVectorizationMetadata(Loop *L) {

    LLVMContext &Ctx = L->getHeader()->getContext();

    MDNode *OldLoopID = L->getLoopID();

    SmallVector<Metadata *, 8> MDs;

    // First operand must refer to the loop metadata node itself.
    MDs.push_back(nullptr);

    // Preserve existing metadata.
    if (OldLoopID) {
      for (unsigned i = 1; i < OldLoopID->getNumOperands(); ++i) {
        MDs.push_back(OldLoopID->getOperand(i));
      }
    }

    Metadata *VectorizeWidthMD[] = {
        MDString::get(Ctx, "llvm.loop.vectorize.width"),
        ConstantAsMetadata::get(
            ConstantInt::get(Type::getInt32Ty(Ctx), VectorizationFactor))};

    MDs.push_back(MDNode::get(Ctx, VectorizeWidthMD));

    Metadata *InterleaveCountMD[] = {
        MDString::get(Ctx, "llvm.loop.interleave.count"),
        ConstantAsMetadata::get(
            ConstantInt::get(Type::getInt32Ty(Ctx), InterleaveFactor))};

    MDs.push_back(MDNode::get(Ctx, InterleaveCountMD));
    Metadata *VectorizeEnableMD[] = {
        MDString::get(Ctx, "llvm.loop.vectorize.enable"),
        ConstantAsMetadata::get(ConstantInt::getTrue(Ctx))};

    MDs.push_back(MDNode::get(Ctx, VectorizeEnableMD));
    MDNode *NewLoopID = MDNode::getDistinct(Ctx, MDs);

    // Make first operand self-reference.
    NewLoopID->replaceOperandWith(0, NewLoopID);

    L->setLoopID(NewLoopID);

    errs() << "Attached vectorization metadata: "
           << "VF=" << VectorizationFactor << ", IF=" << InterleaveFactor
           << "\n";
  }
};

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {

  return {LLVM_PLUGIN_API_VERSION, "LoopVectorizationDatasetGenerator",
          LLVM_VERSION_STRING,

          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "loop-vectorization-dataset-generator") {

                    FPM.addPass(LoopVectorizationDatasetGeneratorPass());

                    return true;
                  }

                  return false;
                });
          }};
}