void test(float *a, float *b, int n) {

  [[clang::annotate("costmodel:unroll:4")]]
  for (int i = 0; i < n; i++) {
    a[i] += b[i];
  }
}