# Compiler Cost Model

A compiler optimization profitability prediction system that combines **LLVM-based program analysis**, **hand-designed heuristics**, and **machine learning** to predict whether applying an optimization to a candidate loop is likely to be profitable.

The system currently supports four loop optimizations:

* Loop Unrolling
* Loop Vectorization
* Loop Tiling
* Loop Fusion

For each optimization, the system extracts compiler-level features from LLVM IR and feeds them into an optimization-specific machine learning model.

A lightweight LLVM heuristic cost model is also provided as a baseline. The two approaches can be run together to compare their decisions on the same annotated optimization candidates.

---

## 1. Problem

Compiler optimizations are not universally beneficial.

An optimization can improve performance for one loop while making another loop slower because of factors such as:

* Loop size
* Memory access behavior
* Control-flow complexity
* Computational intensity
* Cache behavior
* Vectorization characteristics
* Unrolling overhead
* Target-specific instruction costs

Traditional compilers use a combination of static analysis, target-specific cost models, and heuristics to make these decisions.

This project explores whether a **machine learning cost model** can learn these profitability decisions from compiler-level features.

The goal is therefore:

> Given a candidate optimization and its compiler-level characteristics, predict whether applying that optimization is likely to be profitable.

---

# 2. High-Level Architecture

```text
                         Source Code
                             │
                             ▼
                         LLVM Clang
                             │
                             ▼
                         LLVM IR
                             │
                             ▼
                  ┌─────────────────────┐
                  │ LLVM Feature        │
                  │ Extraction Pass     │
                  └──────────┬──────────┘
                             │
                             ▼
                    Feature JSON / Dataset
                             │
                  ┌──────────┴──────────┐
                  │                     │
                  ▼                     ▼
          ML Cost Models          LLVM Heuristic
                  │                     │
                  ▼                     ▼
        Profitability Prediction   Profitability
                  │                     │
                  └──────────┬──────────┘
                             ▼
                       Comparison
```

The important design decision is that **each optimization has its own model**.

```text
                    Candidate Loop
                          │
          ┌───────────────┼────────────────┐
          │               │                │
          ▼               ▼                ▼
       Unroll          Vectorize         Tiling
          │               │                │
          ▼               ▼                ▼
     RF Model         RF Model          RF Model

                         │
                         ▼
                       Fusion
                         │
                         ▼
                     RF Model
```

This is useful because the features and profitability factors are not identical across transformations.

---

# 3. How It Works

The system has two major paths.

## Machine Learning Path

```text
Source
  │
  ▼
LLVM IR
  │
  ▼
Feature Extraction
  │
  ▼
Annotated Optimization Candidates
  │
  ▼
Feature Normalization
  │
  ▼
Optimization-specific ML Model
  │
  ▼
Prediction + Probability
```

## Heuristic Path

```text
Source
  │
  ▼
LLVM IR
  │
  ▼
Annotated Loops
  │
  ▼
LLVM TargetTransformInfo
  │
  ▼
Hand-designed Heuristic
  │
  ▼
Score
  │
  ▼
Profitable / Not Profitable
```

Both paths operate on the **same annotated optimization candidates**, allowing their decisions to be compared directly.

---

# 4. Optimization Annotations

The system does not evaluate every loop for every optimization.

Instead, optimization candidates are explicitly annotated.

For example, a source program can contain annotations conceptually equivalent to:

```c
#pragma ...
for (...) {
    ...
}
```

The compiler passes identify these annotations and associate a loop with an optimization.

The resulting candidate may contain information such as:

```text
optimization = vectorize
function     = kernel
loop         = for.cond
parameters   = {
    vf: 4,
    if: 2
}
```

This is important because the ML model is answering:

> "Is this particular optimization configuration profitable for this particular loop?"

rather than:

> "Is this loop generally profitable to optimize?"

The heuristic model follows the same candidate-selection mechanism.

---

# 5. LLVM Feature Extraction

The core compiler analysis is implemented as LLVM passes.

The passes inspect LLVM IR and collect properties of candidate loops.

Typical features include characteristics such as:

```text
instruction counts
memory operation counts
branch counts
loop depth
loop nesting
trip-count information
load/store characteristics
contiguous memory accesses
strided memory accesses
computational operations
```

Optimization-specific feature extractors are used where necessary.

For example:

### Loop Unrolling

Relevant information can include:

```text
loop size
trip count
instruction count
memory operations
control-flow characteristics
unroll factor
```

### Loop Vectorization

Relevant information can include:

```text
vectorization factor
interleave factor
loads/stores
contiguous accesses
strided accesses
loop characteristics
```

### Loop Tiling

Relevant information can include:

```text
tile size
memory accesses
contiguous accesses
strided accesses
loop structure
```

### Loop Fusion

Relevant information can include:

```text
loop characteristics
instruction counts
memory behavior
dependence-related characteristics
```

The exact feature set is stored with each trained model so that inference uses the same features that were used during training.

---

# 6. Machine Learning Models

The project uses **Random Forest classifiers** for the optimization-specific cost models.

There is one model per optimization:

```text
Unrolling       → Random Forest
Vectorization   → Random Forest
Tiling          → Random Forest
Fusion          → Random Forest
```

Each model predicts a binary profitability label:

```text
0 → Not Profitable
1 → Profitable
```

When supported by the classifier, the system also reports the probability of the positive class.

Example:

```text
VECTORIZE
Probability: 0.5165
Prediction : PROFITABLE
```

The probability should be interpreted as the model's confidence estimate, not as a guaranteed performance improvement.

---

# 7. Heuristic Cost Model

A lightweight LLVM heuristic model is included as a baseline.

It uses LLVM's target-specific instruction cost information through `TargetTransformInfo`.

The heuristic considers:

### Loop Size

Larger loops receive a higher score because there is potentially more work for an optimization to affect.

### Memory Intensity

Memory-heavy loops may benefit from transformations such as:

* Vectorization
* Tiling

### Control Flow

Loops dominated by excessive control flow may be less attractive optimization candidates.

### LLVM Instruction Cost

LLVM's target-specific instruction throughput cost is incorporated into the score.

Conceptually:

```text
                 Loop Features
                      │
        ┌─────────────┼─────────────┐
        ▼             ▼             ▼
     Loop Size    Memory Ratio   Branch Ratio
        │             │             │
        └─────────────┼─────────────┘
                      │
                      ▼
              LLVM Instruction Cost
                      │
                      ▼
                Heuristic Score
                      │
                ┌─────┴─────┐
                ▼           ▼
             >= threshold
                │
                ▼
           PROFITABLE
```

The heuristic is intentionally simple. Its purpose is to provide a transparent baseline against which the ML model can be compared.

---

# 8. ML vs Heuristic Comparison

When the `--heuristic` option is provided, the inference pipeline runs both models.

Example output:

```text
OPTIMIZATION   FUNCTION       LOOP          ML               HEURISTIC
-----------------------------------------------------------------------
TILING         kernel         for.cond      PROFITABLE       PROFITABLE
FUSION         kernel         for.cond21    PROFITABLE       PROFITABLE
UNROLL         kernel         for.cond44    NOT PROFITABLE   PROFITABLE
VECTORIZE      kernel         for.cond56    PROFITABLE       PROFITABLE
```

The system then calculates agreement:

```text
Agreement: 3/4 (75.0%)
```

This comparison is **not an accuracy measurement**.

The heuristic is itself a prediction mechanism and there is currently no independently measured ground-truth performance result involved in this comparison.

Instead, it answers:

> How often does the learned model agree with the compiler-inspired heuristic baseline?

---

# 9. Dataset

Training data is generated from benchmark programs containing annotated optimization candidates.

The dataset pipeline follows:

```text
Benchmark Program
       │
       ▼
Optimization Annotation
       │
       ▼
LLVM Feature Extraction
       │
       ▼
Optimization Configurations
       │
       ▼
Performance / Synthetic Label
       │
       ▼
Processed Dataset
       │
       ▼
Model Training
```

The datasets are organized by optimization.

Example:

```text
data/
├── raw/
│   ├── loop_unroll/
│   └── loop_vectorization/
│
└── processed/
    ├── loop_unroll/
    ├── loop_vectorization/
    ├── loop_tiling/
    └── loop_fusion/
```

The raw datasets preserve extracted compiler information, while processed datasets contain the features in a format suitable for machine learning.

---

# 10. Labels

The classifier requires a profitability label.

A real cost model would ideally obtain labels by measuring execution time:

```text
baseline runtime
        vs
optimized runtime
```

For example:

```text
optimized_time < baseline_time
        │
        ▼
    profitable
```

However, for rapid prototyping and hackathon development, synthetic/assumption-based labels can be used.

Therefore, the current model results should be interpreted as demonstrating the **cost-model pipeline and architecture**, rather than claiming experimentally validated performance prediction accuracy.

This distinction is important when presenting the project.

---

# 11. Repository Structure

A simplified project structure is:

```text
.
├── llvm/
│   └── passes/
│       ├── CompilerCostModel.cpp
│       ├── LLVMHeuristicCostModel.cpp
│       ├── CostModelAnnotation.h
│       ├── LoopUnrollFeatureExtractor.cpp
│       ├── LoopUnrollFeatureExtractor.h
│       ├── LoopVectorizationFeatureExtractor.cpp
│       ├── LoopVectorizationFeatureExtractor.h
│       ├── LoopTilingFeatureExtractor.cpp
│       ├── LoopTilingFeatureExtractor.h
│       ├── LoopFusionFeatureExtractor.cpp
│       ├── LoopFusionFeatureExtractor.h
│       └── CMakeLists.txt
│
├── ml/
│   ├── models/
│   │   ├── loop_unroll_random_forest.joblib
│   │   ├── loop_vectorization_random_forest.joblib
│   │   ├── loop_tiling_random_forest.joblib
│   │   └── loop_fusion_random_forest.joblib
│   │
│   └── requirements.txt
│
├── data/
│   ├── raw/
│   ├── processed/
│   └── results/
│
├── scripts/
│   └── predict.py
│
├── experiments/
├── CMakeLists.txt
└── README.md
```

Build artifacts are generated inside:

```text
build/
```

and should normally not be committed to version control.

---

# 12. Requirements

The project requires:

* Linux environment
* CMake
* Ninja or Make
* Python 3
* LLVM/Clang
* LLVM development headers/libraries
* Git

Python dependencies include:

```text
joblib
pandas
scikit-learn
```

Install them using:

```bash
python3 -m pip install -r ml/requirements.txt
```

Run this command from the **project root**.

---

# 13. LLVM Setup

The project expects a locally built LLVM installation.

The required LLVM binaries are:

```text
clang
opt
```

The project currently expects them in a sibling LLVM directory:

```text
../llvm/
```

with binaries at:

```text
../llvm/build/bin/clang
../llvm/build/bin/opt
```

If LLVM is installed somewhere else, update the LLVM path in:

```text
scripts/predict.py
```

---

# 14. Build the LLVM Passes

From the **project root**:

```bash
cmake -S . -B build
```

Then:

```bash
cmake --build build -j$(nproc)
```

The generated plugins will be placed under:

```text
build/llvm/passes/
```

Important plugins include:

```text
CompilerCostModel.so
LLVMHeuristicCostModel.so
```

---

# 15. Running the Complete Pipeline

The main inference script is:

```text
scripts/predict.py
```

From the **project root**, run:

```bash
python3 scripts/predict.py path/to/program.c
```

The pipeline performs:

```text
1. Compile source → LLVM IR
2. Extract compiler features
3. Load optimization-specific ML models
4. Generate ML predictions
5. Print predictions
```

---

# 16. Running with the Heuristic Baseline

To run both the ML model and LLVM heuristic:

From the **project root**:

```bash
python3 scripts/predict.py path/to/program.c --heuristic
```

The output contains:

```text
ML PREDICTIONS
```

followed by:

```text
LLVM HEURISTIC PREDICTIONS
```

and finally:

```text
ML vs LLVM HEURISTIC COMPARISON
```

The comparison is performed only for the same annotated optimization candidates.

---

# 17. Example

Running the pipeline on a benchmark can produce output similar to:

```text
ML PREDICTIONS
==========================================================================================

OPTIMIZATION   FUNCTION            LOOP                PARAMETERS
------------------------------------------------------------------------------------------
TILING         kernel              for.cond            {'tile_size': 32}
FUSION         kernel              for.cond21          {}
UNROLL         kernel              for.cond44          {'factor': 4}
VECTORIZE      kernel              for.cond56          {'if': 2, 'vf': 4}
```

The corresponding predictions may be:

```text
TILING       → PROFITABLE
FUSION       → PROFITABLE
UNROLL       → NOT PROFITABLE
VECTORIZE    → PROFITABLE
```

The heuristic model evaluates those same four candidates and produces its own decisions.

---

# 18. Directly Running the Heuristic Pass

The heuristic pass can also be invoked directly using LLVM's `opt`.

From the **project root**:

```bash
../llvm/build/bin/opt \
  -load-pass-plugin ./build/llvm/passes/LLVMHeuristicCostModel.so \
  -passes="function(llvm-heuristic-cost-model)" \
  data/results/program.ll \
  -disable-output
```

The pass prints information such as:

```text
Optimization : vectorize
Loop         : for.cond

instructions : 39
memory ops   : 23
branches     : 7
LLVM cost    : 34
memory ratio : 0.5897
branch ratio : 0.1795
score        : 0.9000
profitable   : YES
```

Only annotated optimization candidates are evaluated.

---

# 19. Why Optimization-Specific Models?

A single model could theoretically predict profitability for all transformations.

However, different transformations depend on different characteristics.

For example:

```text
Unrolling
    → trip count
    → loop size
    → instruction overhead

Vectorization
    → memory access patterns
    → vectorization factor
    → interleaving
    → SIMD suitability

Tiling
    → memory locality
    → tile size
    → access patterns

Fusion
    → loop structure
    → memory behavior
    → compatibility between loops
```

Therefore, this project uses:

```text
                Cost Model
                    │
       ┌────────────┼────────────┐
       │            │            │
    Unroll      Vectorize      Tiling
       │            │            │
       ▼            ▼            ▼
       RF           RF           RF

                    │
                    ▼
                  Fusion
                    │
                    ▼
                    RF
```

This keeps each model relatively simple and makes its behavior easier to explain.

---

# 20. Design Philosophy

The project intentionally combines three layers:

### Compiler Analysis

LLVM provides accurate low-level program information.

```text
LLVM IR
  ↓
Loop structure
  ↓
Instructions
  ↓
Memory behavior
  ↓
Target costs
```

### Machine Learning

ML learns relationships between compiler features and profitability labels.

```text
features → trained model → prediction
```

### Heuristics

A transparent baseline provides an interpretable alternative.

```text
features → rules → score → decision
```

This combination makes the system easier to explain than a purely black-box ML approach.

---

# 21. Limitations

The current implementation is a prototype.

Important limitations include:


### Limited Training Data

The quality of a learned cost model depends heavily on:

* Number of benchmarks
* Diversity of loops
* Optimization configurations
* Hardware targets
* Label quality

### Binary Prediction

The current models primarily answer:

```text
profitable / not profitable
```

rather than predicting the exact speedup.

### Limited Hardware Awareness

LLVM's target instruction cost is incorporated into the heuristic model, but the ML models are not yet fully hardware-specific.

A production-quality system would likely train separate models or include hardware features.

### Candidate Generation

The current system relies on annotated optimization candidates rather than automatically discovering every possible transformation.

---

# 22. Future Work

Several improvements can extend the system.


## Speedup Prediction

Instead of:

```text
PROFITABLE
```

predict:

```text
expected speedup = 1.23x
```

This would allow optimization configurations to be ranked.

## Configuration Ranking

For example:

```text
Vectorization

VF=4   → 1.12x
VF=8   → 1.19x
VF=16  → 0.97x
```

The compiler could then choose the highest-scoring configuration.

## More Optimizations

Potential future transformations include:

* Loop interchange
* Loop distribution
* Software pipelining
* Function inlining
* Partial unrolling
* Instruction scheduling

## Hardware-Aware Models

Additional model inputs could include:

```text
CPU architecture
cache sizes
SIMD width
number of cores
memory bandwidth
target ISA
```

This would allow the model to adapt its predictions to different machines.

---

# 23. Overall Pipeline

The complete system can be summarized as:

```text
                         SOURCE PROGRAM
                               │
                               ▼
                            CLANG
                               │
                               ▼
                           LLVM IR
                               │
                               ▼
                 ┌─────────────────────────┐
                 │ LLVM Compiler Analysis  │
                 └────────────┬────────────┘
                              │
                              ▼
                    Annotated Loop Candidates
                              │
             ┌────────────────┴────────────────┐
             │                                 │
             ▼                                 ▼
      Feature Extraction                LLVM Heuristic
             │                                 │
             ▼                                 ▼
       Feature Dataset                    LLVM TTI Cost
             │                                 │
             ▼                                 ▼
      Optimization-specific              Hand-designed
       Random Forest Model                 heuristic
             │                                 │
             ▼                                 ▼
        ML Prediction                    Heuristic Score
             │                                 │
             └───────────────┬─────────────────┘
                             ▼
                         COMPARISON
                             │
                             ▼
                   PROFITABILITY DECISION
```

---

# 24. Summary

This project demonstrates a compiler cost-model architecture combining:

**LLVM**

for extracting low-level program and target information,

**Machine Learning**

for learning optimization profitability from compiler features,

and **Heuristics**

for providing a transparent compiler-inspired baseline.

The system currently supports:

```text
Loop Unrolling
Loop Vectorization
Loop Tiling
Loop Fusion
```

and can run ML predictions and LLVM heuristic predictions on the same annotated candidates.

The long-term goal is to evolve this prototype into a system capable of **predicting and ranking optimization configurations based on expected performance benefit**, allowing the compiler to make more informed optimization decisions.
