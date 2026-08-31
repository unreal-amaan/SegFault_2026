#include <stdio.h>

int add(int a, int b) { return a + b; }

int multiply(int a, int b) { return a * b; }

int main() {
  int x = add(2, 3);
  int y = multiply(x, 10);

  printf("%d\n", y);

  return 0;
}