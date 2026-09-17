#define COSTMODEL_UNROLL(x) __costmodel_unroll(x)

extern void __costmodel_unroll(int);

void test(float *a, float *b, int n) {

  COSTMODEL_UNROLL(4);

  for (int i = 0; i < n; i++) {
    a[i] = a[i] + b[i];
  }
}