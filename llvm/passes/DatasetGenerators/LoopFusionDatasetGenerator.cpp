#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"

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

// -----------------------------------------------------------------------------
// Command-line options
// -----------------------------------------------------------------------------

static cl::opt<unsigned> TargetLoop1ID("target-loop1-id",
                                       cl::desc("First loop ID to fuse"),
                                       cl::init(0));

static cl::opt<unsigned> TargetLoop2ID("target-loop2-id",
                                       cl::desc("Second loop ID to fuse"),
                                       cl::init(1));

// -----------------------------------------------------------------------------
// LoopFusionDatasetGeneratorPass
//
// Responsibility:
//
//   1. Discover top-level loops.
//   2. Assign the same persistent loop IDs used by
//      LoopFusionFeatureExtractor.
//   3. Find the requested pair of loops.
//   4. Validate whether the pair is a supported fusion candidate.
//   5. Attach:
//
//        compiler_cost_model.fuse_with
//
// The pass does NOT:
//   - extract ML features
//   - measure runtime
//
// Those belong to later stages of the dataset pipeline.
// -----------------------------------------------------------------------------

class LoopFusionDatasetGeneratorPass
    : public PassInfoMixin<LoopFusionDatasetGeneratorPass> {

private:
  struct SelectedLoops {
    Loop *Loop1 = nullptr;
    Loop *Loop2 = nullptr;
  };

  // ---------------------------------------------------------------------------
  // Read compiler_cost_model.loop_id from LoopID metadata.
  // ---------------------------------------------------------------------------

  std::optional<unsigned> getLoopID(Loop *L) {

    if (!L)
      return std::nullopt;

    MDNode *LoopMetadata = L->getLoopID();

    if (!LoopMetadata)
      return std::nullopt;

    // Operand 0 is the self-reference of the loop metadata node.
    for (unsigned I = 1; I < LoopMetadata->getNumOperands(); ++I) {

      MDNode *Metadata = dyn_cast<MDNode>(LoopMetadata->getOperand(I));

      if (!Metadata || Metadata->getNumOperands() < 2)
        continue;

      MDString *Name = dyn_cast<MDString>(Metadata->getOperand(0));

      if (!Name)
        continue;

      if (Name->getString() != "compiler_cost_model.loop_id")
        continue;

      ConstantInt *IDValue =
          mdconst::dyn_extract<ConstantInt>(Metadata->getOperand(1));

      if (!IDValue)
        continue;

      return static_cast<unsigned>(IDValue->getZExtValue());
    }

    return std::nullopt;
  }

  // ---------------------------------------------------------------------------
  // Attach compiler_cost_model.loop_id.
  //
  // This intentionally mirrors LoopFusionFeatureExtractor.
  // ----------------------------------------------------------------------------

  void assignLoopID(Loop *L, unsigned ID) {

    if (!L)
      return;

    LLVMContext &Ctx = L->getHeader()->getContext();

    MDNode *OldLoopID = L->getLoopID();

    SmallVector<Metadata *, 8> MDs;

    // Operand 0 must refer to the loop metadata node itself.
    MDs.push_back(nullptr);

    // Preserve existing loop metadata.
    if (OldLoopID) {

      for (unsigned I = 1; I < OldLoopID->getNumOperands(); ++I) {

        Metadata *Operand = OldLoopID->getOperand(I);

        bool IsLoopIDMetadata = false;

        if (auto *MD = dyn_cast<MDNode>(Operand)) {

          if (MD->getNumOperands() >= 2) {

            if (auto *Name = dyn_cast<MDString>(MD->getOperand(0))) {

              if (Name->getString() == "compiler_cost_model.loop_id") {

                IsLoopIDMetadata = true;
              }
            }
          }
        }

        // Do not duplicate the loop ID metadata.
        if (!IsLoopIDMetadata)
          MDs.push_back(Operand);
      }
    }

    Metadata *LoopIDMD[] = {

        MDString::get(Ctx, "compiler_cost_model.loop_id"),

        ConstantAsMetadata::get(ConstantInt::get(Type::getInt32Ty(Ctx), ID))};

    MDs.push_back(MDNode::get(Ctx, LoopIDMD));

    MDNode *NewLoopID = MDNode::getDistinct(Ctx, MDs);

    NewLoopID->replaceOperandWith(0, NewLoopID);

    L->setLoopID(NewLoopID);
  }

  // ---------------------------------------------------------------------------
  // Discover top-level loops and assign IDs.
  //
  // IMPORTANT:
  //
  // This mirrors LoopFusionFeatureExtractor:
  //
  //   for (Loop *L : LI)
  //       if (L->getParentLoop() == nullptr)
  //           Loops.push_back(L);
  //
  // IDs therefore have the same ordering:
  //
  //   first top-level loop  -> 0
  //   second top-level loop -> 1
  //   third top-level loop  -> 2
  //   ...
  // ---------------------------------------------------------------------------

  SmallVector<Loop *, 8> discoverAndAssignLoopIDs(LoopInfo &LI) {

    SmallVector<Loop *, 8> Loops;

    unsigned NextLoopID = 0;

    for (Loop *L : LI) {

      if (L->getParentLoop() != nullptr)
        continue;

      Loops.push_back(L);
    }

    errs() << "\nDiscovered top-level loops:\n";

    for (Loop *L : Loops) {

      unsigned ID = NextLoopID++;

      assignLoopID(L, ID);

      errs() << "  Assigned loop ID " << ID
             << " to loop header: " << L->getHeader()->getName() << "\n";
    }

    return Loops;
  }

  // ---------------------------------------------------------------------------
  // Find loops with the requested IDs.
  // -----------------------------------------------------------------------------

  void findLoops(ArrayRef<Loop *> Loops, SelectedLoops &Selected) {

    for (Loop *L : Loops) {

      std::optional<unsigned> ID = getLoopID(L);

      if (!ID)
        continue;

      errs() << "Found loop ID: " << *ID << " (" << L->getHeader()->getName()
             << ")\n";

      if (*ID == TargetLoop1ID) {

        Selected.Loop1 = L;

        errs() << "  Selected as loop 1\n";
      }

      if (*ID == TargetLoop2ID) {

        Selected.Loop2 = L;

        errs() << "  Selected as loop 2\n";
      }
    }
  }

  // ---------------------------------------------------------------------------
  // Check whether L1 is followed by L2 in the CFG.
  // -----------------------------------------------------------------------------

  bool areAdjacent(Loop *L1, Loop *L2) {

    if (!L1 || !L2)
      return false;

    BasicBlock *L2Header = L2->getHeader();

    if (!L2Header)
      return false;

    SmallVector<BasicBlock *, 4> ExitingBlocks;

    L1->getExitingBlocks(ExitingBlocks);

    for (BasicBlock *ExitBB : ExitingBlocks) {

      SmallVector<BasicBlock *, 8> Worklist;

      SmallPtrSet<BasicBlock *, 8> Visited;

      Worklist.push_back(ExitBB);

      while (!Worklist.empty()) {

        BasicBlock *BB = Worklist.pop_back_val();

        if (!Visited.insert(BB).second)
          continue;

        for (BasicBlock *Successor : successors(BB)) {

          if (Successor == L2Header)
            return true;

          // Do not walk through L2 itself.
          if (L2->contains(Successor))
            continue;

          Worklist.push_back(Successor);
        }
      }
    }

    return false;
  }

  // ---------------------------------------------------------------------------
  // Restrict first dataset version to simple canonical loops.
  //
  // Requirements:
  //
  //   - exactly one exiting block
  //   - exactly one latch
  //   - valid header
  // ----------------------------------------------------------------------------

  bool hasSimpleLoopShape(Loop *L) {

    if (!L)
      return false;

    SmallVector<BasicBlock *, 4> ExitingBlocks;

    L->getExitingBlocks(ExitingBlocks);

    if (ExitingBlocks.size() != 1)
      return false;

    SmallVector<BasicBlock *, 4> Latches;

    L->getLoopLatches(Latches);

    if (Latches.size() != 1)
      return false;

    BasicBlock *Header = L->getHeader();

    if (!Header)
      return false;

    if (pred_begin(Header) == pred_end(Header))
      return false;

    return true;
  }

  // ---------------------------------------------------------------------------
  // Ensure both loops belong to the same function.
  // -----------------------------------------------------------------------------

  bool sameParentFunction(Loop *L1, Loop *L2) {

    if (!L1 || !L2)
      return false;

    return L1->getHeader()->getParent() == L2->getHeader()->getParent();
  }

  // ---------------------------------------------------------------------------
  // Attach:
  //
  //   compiler_cost_model.fuse_with = <other loop ID>
  //
  // while preserving all existing loop metadata.
  // -----------------------------------------------------------------------------

  void attachFusionMetadata(Loop *L1, unsigned Loop1ID, Loop *L2,
                            unsigned Loop2ID) {

    auto attachToLoop = [](Loop *L, unsigned FuseWithID) {
      LLVMContext &Ctx = L->getHeader()->getContext();

      MDNode *OldLoopID = L->getLoopID();

      SmallVector<Metadata *, 8> MDs;

      // Operand 0 is the self-reference.
      MDs.push_back(nullptr);

      // Preserve existing metadata.
      if (OldLoopID) {

        for (unsigned I = 1; I < OldLoopID->getNumOperands(); ++I) {

          Metadata *Operand = OldLoopID->getOperand(I);

          bool IsExistingFusionMetadata = false;

          if (auto *MD = dyn_cast<MDNode>(Operand)) {

            if (MD->getNumOperands() >= 2) {

              if (auto *Name = dyn_cast<MDString>(MD->getOperand(0))) {

                if (Name->getString() == "compiler_cost_model.fuse_with") {

                  IsExistingFusionMetadata = true;
                }
              }
            }
          }

          if (!IsExistingFusionMetadata)
            MDs.push_back(Operand);
        }
      }

      Metadata *FusionMD[] = {

          MDString::get(Ctx, "compiler_cost_model.fuse_with"),

          ConstantAsMetadata::get(
              ConstantInt::get(Type::getInt32Ty(Ctx), FuseWithID))};

      MDs.push_back(MDNode::get(Ctx, FusionMD));

      MDNode *NewLoopID = MDNode::getDistinct(Ctx, MDs);

      NewLoopID->replaceOperandWith(0, NewLoopID);

      L->setLoopID(NewLoopID);
    };

    attachToLoop(L1, Loop2ID);
    attachToLoop(L2, Loop1ID);

    errs() << "\nAttached fusion metadata:\n";

    errs() << "  loop " << Loop1ID << " <-> loop " << Loop2ID << "\n";
  }

  // ---------------------------------------------------------------------------
  // Validate candidate.
  // -----------------------------------------------------------------------------

  bool validateCandidate(Loop *L1, Loop *L2, ScalarEvolution &SE) {

    if (!L1 || !L2) {

      errs() << "ERROR: target loop(s) not found\n";

      return false;
    }

    if (L1 == L2) {

      errs() << "ERROR: cannot fuse "
                "a loop with itself\n";

      return false;
    }

    if (!sameParentFunction(L1, L2)) {

      errs() << "ERROR: loops belong "
                "to different functions\n";

      return false;
    }

    // First dataset version only considers
    // top-level loops.
    if (L1->getParentLoop() != nullptr || L2->getParentLoop() != nullptr) {

      errs() << "ERROR: nested loops "
                "are not supported yet\n";

      return false;
    }

    if (!hasSimpleLoopShape(L1) || !hasSimpleLoopShape(L2)) {

      errs() << "ERROR: loop shape "
                "is not supported\n";

      errs() << "       required: one latch "
                "and one exiting block\n";

      return false;
    }

    if (!areAdjacent(L1, L2)) {

      errs() << "ERROR: selected loops "
                "are not adjacent\n";

      return false;
    }

    // Fusion requires compatible iteration
    // spaces in the first dataset version.
    const SCEV *Trip1 = SE.getBackedgeTakenCount(L1);

    const SCEV *Trip2 = SE.getBackedgeTakenCount(L2);

    if (isa<SCEVCouldNotCompute>(Trip1) || isa<SCEVCouldNotCompute>(Trip2)) {

      errs() << "ERROR: could not determine "
                "trip count\n";

      return false;
    }

    if (Trip1 != Trip2) {

      errs() << "ERROR: trip counts differ\n";

      errs() << "  loop1 trip count: " << *Trip1 << "\n";

      errs() << "  loop2 trip count: " << *Trip2 << "\n";

      return false;
    }

    return true;
  }

public:
  // ---------------------------------------------------------------------------
  // Pass entry point.
  // -----------------------------------------------------------------------------

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {

    if (F.isDeclaration())
      return PreservedAnalyses::all();

    errs() << "\n";
    errs() << "========================================\n";
    errs() << "LoopFusionDatasetGenerator\n";
    errs() << "========================================\n";

    errs() << "Function: " << F.getName() << "\n";

    errs() << "Target loop 1: " << TargetLoop1ID << "\n";

    errs() << "Target loop 2: " << TargetLoop2ID << "\n";

    LoopInfo &LI = AM.getResult<LoopAnalysis>(F);

    ScalarEvolution &SE = AM.getResult<ScalarEvolutionAnalysis>(F);

    // -------------------------------------------------------------------------
    // IMPORTANT:
    //
    // Assign IDs in exactly the same way as
    // LoopFusionFeatureExtractor.
    // -------------------------------------------------------------------------

    SmallVector<Loop *, 8> Loops = discoverAndAssignLoopIDs(LI);

    // -------------------------------------------------------------------------
    // Find requested loops.
    // -------------------------------------------------------------------------

    SelectedLoops Selected;

    findLoops(Loops, Selected);

    if (!Selected.Loop1 || !Selected.Loop2) {

      errs() << "\nERROR: could not find "
                "both target loops\n";

      if (!Selected.Loop1)

        errs() << "  Missing loop ID: " << TargetLoop1ID << "\n";

      if (!Selected.Loop2)

        errs() << "  Missing loop ID: " << TargetLoop2ID << "\n";

      errs() << "========================================\n";

      return PreservedAnalyses::all();
    }

    Loop *L1 = Selected.Loop1;

    Loop *L2 = Selected.Loop2;

    // -------------------------------------------------------------------------
    // Normalize ordering so L1 appears before L2.
    // -------------------------------------------------------------------------

    if (areAdjacent(L2, L1))
      std::swap(L1, L2);

    std::optional<unsigned> L1ID = getLoopID(L1);

    std::optional<unsigned> L2ID = getLoopID(L2);

    if (!L1ID || !L2ID) {

      errs() << "ERROR: selected loops "
                "do not have loop IDs\n";

      return PreservedAnalyses::all();
    }

    // -------------------------------------------------------------------------
    // Validate candidate.
    // -------------------------------------------------------------------------

    if (!validateCandidate(L1, L2, SE)) {

      errs() << "\nCandidate rejected.\n";
      errs() << "========================================\n";

      return PreservedAnalyses::all();
    }

    // -------------------------------------------------------------------------
    // Candidate accepted.
    // -------------------------------------------------------------------------

    unsigned TripCount1 = SE.getSmallConstantTripCount(L1);

    unsigned TripCount2 = SE.getSmallConstantTripCount(L2);

    bool TripCountKnown = TripCount1 != 0 && TripCount2 != 0;

    errs() << "\nCandidate accepted.\n";

    errs() << "  loop1 ID: " << *L1ID << "\n";

    errs() << "  loop2 ID: " << *L2ID << "\n";

    errs() << "  trip_count_known: " << TripCountKnown << "\n";

    if (TripCountKnown) {

      errs() << "  trip_count: " << TripCount1 << "\n";
    }

    errs() << "  adjacent: yes\n";
    errs() << "  simple shape: yes\n";

    // -------------------------------------------------------------------------
    // Attach fusion relationship.
    // -------------------------------------------------------------------------

    attachFusionMetadata(L1, *L1ID, L2, *L2ID);

    errs() << "\nFusion candidate prepared.\n";

    errs() << "========================================\n";

    return PreservedAnalyses::none();
  }
};

// -----------------------------------------------------------------------------
// LLVM plugin registration
// -----------------------------------------------------------------------------

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {

  return {

      LLVM_PLUGIN_API_VERSION,

      "LoopFusionDatasetGenerator",

      LLVM_VERSION_STRING,

      [](PassBuilder &PB) {
        PB.registerPipelineParsingCallback(

            [](StringRef Name, FunctionPassManager &FPM,
               ArrayRef<PassBuilder::PipelineElement>) {
              if (Name == "loop-fusion-dataset-generator") {

                FPM.addPass(LoopFusionDatasetGeneratorPass());

                return true;
              }

              return false;
            });
      }};
}
