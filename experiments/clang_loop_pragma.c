void test(float *a, int n) {

#pragma clang loop unroll_count(4)
  for (int i = 0; i < n; i++) {
    a[i] += 1.0f;
  }
}