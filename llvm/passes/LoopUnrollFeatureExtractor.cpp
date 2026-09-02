#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Plugins/PassPlugin.h"
#include <llvm/Analysis/IVDescriptors.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/Analysis.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

struct LoopUnrollFeatures {
  unsigned loopID;

  // Loop structure
  unsigned loopDepth;
  bool isInnermost;
  bool hasParentLoop;
  unsigned numSubLoops;
  unsigned numBasicBlocks;
  unsigned numExitingBlocks;

  // Loop body
  unsigned numInstructions;
  unsigned numPhiNodes;
  unsigned numTerminatorInstructions;

  // Instruction mix
  unsigned numLoads;
  unsigned numStores;
  unsigned numIntegerOps;
  unsigned numFloatOps;
  unsigned numBranches;
  unsigned numConditionalBranches;
  unsigned numCalls;

  // Trip count
  bool tripCountKnown;
  uint64_t tripCount;
  bool maxTripCountKnown;
  uint64_t maxTripCount;

  // Derived features
  double memoryOpRatio;
  double controlOverheadRatio;
  double arithmeticIntensity;
};
unsigned LoopID = 0;

class LoopUnrollFeatureExtractor
    : public PassInfoMixin<LoopUnrollFeatureExtractor> {
private:
  void extractLoopFeatures(Loop *L, ScalarEvolution &SE) {
    LoopUnrollFeatures Features{};

    unsigned CurrentLoopID = LoopID++;
    Features.loopID = CurrentLoopID;
    attachLoopID(L, CurrentLoopID);

    Features.loopDepth = L->getLoopDepth();
    Features.isInnermost = L->isInnermost();
    Features.hasParentLoop = L->getParentLoop() != nullptr;
    Features.numSubLoops = L->getSubLoops().size();
    Features.numBasicBlocks = L->getNumBlocks();

    SmallVector<BasicBlock *, 4> ExitingBlocks;
    L->getExitingBlocks(ExitingBlocks);

    Features.numExitingBlocks = ExitingBlocks.size();

    unsigned numInstructions = 0;
    unsigned numPhiNodes = 0;
    unsigned numTerminatorInstructions = 0;

    unsigned numLoads = 0;
    unsigned numStores = 0;
    unsigned numIntegerOps = 0;
    unsigned numFloatOps = 0;
    unsigned numBranches = 0;
    unsigned numConditionalBranches = 0;
    unsigned numCalls = 0;

    for (BasicBlock *BB : L->blocks()) {
      for (Instruction &Inst : *BB) {
        numInstructions++;

        if (isa<PHINode>(&Inst)) {
          numPhiNodes++;
        }
        if (Inst.isTerminator()) {
          numTerminatorInstructions++;
        }
        if (isa<LoadInst>(&Inst)) {
          numLoads++;
        }
        if (isa<StoreInst>(&Inst)) {
          numStores++;
        }
        if (auto *BI = dyn_cast<BranchInst>(&Inst)) {
          numBranches++;

          if (BI->isConditional()) {
            numConditionalBranches++;
          }
        }
        if (isa<CallBase>(&Inst)) {
          numCalls++;
        }

        switch (Inst.getOpcode()) {
        case Instruction::Add:
        case Instruction::Sub:
        case Instruction::Mul:
        case Instruction::SDiv:
        case Instruction::UDiv:
        case Instruction::SRem:
        case Instruction::URem:
          numIntegerOps++;
          break;
        case Instruction::FAdd:
        case Instruction::FSub:
        case Instruction::FMul:
        case Instruction::FDiv:
        case Instruction::FRem:
          numFloatOps++;
          break;
        default:
          break;
        }
      }
    }

    Features.numInstructions = numInstructions;
    Features.numPhiNodes = numPhiNodes;
    Features.numTerminatorInstructions = numTerminatorInstructions;
    Features.numLoads = numLoads;
    Features.numStores = numStores;
    Features.numIntegerOps = numIntegerOps;
    Features.numFloatOps = numFloatOps;
    Features.numBranches = numBranches;
    Features.numConditionalBranches = numConditionalBranches;
    Features.numCalls = numCalls;

    unsigned tripCounts = SE.getSmallConstantTripCount(L);

    Features.tripCountKnown = tripCounts != 0;
    Features.tripCount = tripCounts;

    unsigned maxTripCounts = SE.getSmallConstantMaxTripCount(L);

    Features.maxTripCountKnown = maxTripCounts != 0;
    Features.maxTripCount = maxTripCounts;

    unsigned memoryOps = numLoads + numStores;

    unsigned arithmeticOps = numIntegerOps + numFloatOps;

    unsigned controlOps = numBranches + numTerminatorInstructions;

    Features.memoryOpRatio =
        numInstructions > 0 ? static_cast<double>(memoryOps) / numInstructions
                            : 0.0;
    Features.controlOverheadRatio =
        numInstructions > 0 ? static_cast<double>(numBranches) / numInstructions
                            : 0.0;
    Features.arithmeticIntensity =
        memoryOps > 0 ? static_cast<double>(arithmeticOps) / memoryOps
                      : static_cast<double>(arithmeticOps);

    printInfo(Features);

    // Recursively process nested loops
    for (Loop *SubLoop : *L) {
      extractLoopFeatures(SubLoop, SE);
    }
  }

  void attachLoopID(Loop *L, unsigned LoopNumber) {
    LLVMContext &Ctx = L->getHeader()->getContext();

    SmallVector<Metadata *, 8> Operands;

    Operands.push_back(nullptr);

    if (MDNode *ExistingLoopID = L->getLoopID()) {
      for (unsigned I = 1; I < ExistingLoopID->getNumOperands(); ++I) {
        Operands.push_back(ExistingLoopID->getOperand(I));
      }
    }

    Metadata *LoopIDOperands[] = {
        MDString::get(Ctx, "compiler_cost_model.loop_id"),
        ConstantAsMetadata::get(
            ConstantInt::get(Type::getInt32Ty(Ctx), LoopNumber))};

    Operands.push_back(MDNode::get(Ctx, LoopIDOperands));

    MDNode *NewLoopID = MDNode::getDistinct(Ctx, Operands);

    NewLoopID->replaceOperandWith(0, NewLoopID);

    L->setLoopID(NewLoopID);
  }

  void printInfo(LoopUnrollFeatures &Features) {

    errs() << "\nLoop ID: " << Features.loopID << "\n";
    errs() << "  loop_depth: " << Features.loopDepth << "\n";
    errs() << "  is_innermost: " << Features.isInnermost << "\n";
    errs() << "  has_parent_loop: " << Features.hasParentLoop << "\n";
    errs() << "  num_subloops: " << Features.numSubLoops << "\n";
    errs() << "  num_basic_blocks: " << Features.numBasicBlocks << "\n";
    errs() << "  num_exiting_blocks: " << Features.numExitingBlocks << "\n";
    errs() << "  num_instructions: " << Features.numInstructions << "\n";
    errs() << "  num_phi_nodes: " << Features.numPhiNodes << "\n";
    errs() << "  num_terminator_instructions: "
           << Features.numTerminatorInstructions << "\n";
    errs() << "  num_loads: " << Features.numLoads << "\n";
    errs() << "  num_stores: " << Features.numStores << "\n";
    errs() << "  num_integer_ops: " << Features.numIntegerOps << "\n";
    errs() << "  num_float_ops: " << Features.numFloatOps << "\n";
    errs() << "  num_branches: " << Features.numBranches << "\n";
    errs() << "  num_conditional_branches: " << Features.numConditionalBranches
           << "\n";
    errs() << "  num_calls: " << Features.numCalls << "\n";
    errs() << "  trip_count_known: " << Features.tripCountKnown << "\n";
    errs() << "  trip_count: " << Features.tripCount << "\n";
    errs() << "  max_trip_count_known: " << Features.maxTripCountKnown << "\n";
    errs() << "  max_trip_count: " << Features.maxTripCount << "\n";
  
    errs() << "  memory_op_ratio: " << Features.memoryOpRatio << "\n";
    errs() << "  control_overhead_ratio: " << Features.controlOverheadRatio
           << "\n";
    errs() << "  arithmetic_intensity: " << Features.arithmeticIntensity
           << "\n";
  }

public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {

    errs() << "\n---LoopUnrollFeatureExtractor Invoked---" << "\n";

    if (F.isDeclaration()) {
      return PreservedAnalyses::all();
    }
    LoopInfo &LI = AM.getResult<LoopAnalysis>(F);
    ScalarEvolution &SE =
        AM.getResult<ScalarEvolutionAnalysis>(F); // for trip counts

    errs() << "\nFunction:" << F.getName() << "\n";

    // unsigned LoopID = 0;

    for (Loop *L : LI) {
      extractLoopFeatures(L,  SE);
    }

    return PreservedAnalyses::none();
  }
};

extern "C" LLVM_ATTRIBUTE_WEAK ::PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "LoopUnrollFeatureExtractor",
          LLVM_VERSION_STRING, [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "loop-unroll-feature-extractor-pass") {
                    FPM.addPass(LoopUnrollFeatureExtractor());
                    return true;
                  }
                  return false;
                });
          }};
}