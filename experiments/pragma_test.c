void test(float *a, float *b, int n) {

#pragma costmodel unroll factor = 4

  for (int i = 0; i < n; i++) {
    a[i] += b[i];
  }
}