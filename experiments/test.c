int main() {
  volatile int result = 0;

  for (int i = 0; i < 5; i++) {
    result += i;

    for (int j = 1; j < 6; ++j) {
      result += i * j;
      result += j;
    }
  }

  return result;
}