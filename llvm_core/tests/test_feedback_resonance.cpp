// cpp_core/tests/test_feedback_resonance.cpp
int recursive_function(int n) {
  if (n <= 0) {
    return 1;
  }
  return n * recursive_function(n - 1);
}

int main() { return recursive_function(5); }
