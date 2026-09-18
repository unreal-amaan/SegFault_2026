add_llvm_pass_plugin(CompilerCostModel
    CompilerCostModel.cpp
    LoopUnrollFeatureExtractor.cpp
    LoopVectorizationFeatureExtractor.cpp
    LoopFusionFeatureExtractor.cpp
    LoopTilingFeatureExtractor.cpp
    PARTIAL_SOURCES_INTENDED
)

add_llvm_pass_plugin(LLVMHeuristicCostModel
    LLVMHeuristicCostModel.cpp
    PARTIAL_SOURCES_INTENDED
)
