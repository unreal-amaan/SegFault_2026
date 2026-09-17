#pragma once

namespace costmodel {

enum class Optimization { Unroll, Vectorize, Tiling, Fusion };

struct Request {
  Optimization optimization;
  int parameter = 0;
};

} // namespace costmodel

#define COSTMODEL_UNROLL(x) __costmodel_unroll(x)

#define COSTMODEL_VECTORIZE(vf, interleave)                                    \
  __costmodel_vectorize(vf, interleave)

#define COSTMODEL_TILING(x) __costmodel_tiling(x)

#define COSTMODEL_FUSION() __costmodel_fusion()