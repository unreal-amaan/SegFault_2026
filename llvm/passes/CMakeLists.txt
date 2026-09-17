add_llvm_pass_plugin(CompilerCostModel
    CompilerCostModel.cpp
    PARTIAL_SOURCES_INTENDED
)

add_llvm_pass_plugin(LoopUnrollFeatureExtractor
    LoopUnrollFeatureExtractor.cpp
    PARTIAL_SOURCES_INTENDED
)


add_llvm_pass_plugin(LoopVectorizationFeatureExtractor
    LoopVectorizationFeatureExtractor.cpp
    PARTIAL_SOURCES_INTENDED
)

add_llvm_pass_plugin(LoopFusionFeatureExtractor
    LoopFusionFeatureExtractor.cpp
    PARTIAL_SOURCES_INTENDED
)

add_llvm_pass_plugin(LoopTilingFeatureExtractor
    LoopTilingFeatureExtractor.cpp
    PARTIAL_SOURCES_INTENDED
)

add_llvm_pass_plugin(LoopUnrollDatasetGenerator
    DatasetGenerators/LoopUnrollDatasetGenerator.cpp
    PARTIAL_SOURCES_INTENDED
)

add_llvm_pass_plugin(LoopVectorizationDatasetGenerator
    DatasetGenerators/LoopVectorizationDatasetGenerator.cpp
    PARTIAL_SOURCES_INTENDED
)

add_llvm_pass_plugin(LoopFusionDatasetGenerator
    DatasetGenerators/LoopFusionDatasetGenerator.cpp
    PARTIAL_SOURCES_INTENDED
)