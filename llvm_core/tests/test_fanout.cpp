int g = 0;

void a(int *p) { *p = 1; }

void b() { a(&g); }

void c() {
  int x = 0;
  a(&x);
}

void d() {
  // no calls
}

int e() {
  b();
  c();
  return 0;
}
