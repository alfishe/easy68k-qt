/* Solution C-12-EX1: constexpr fib(10) (55) + scale_pow2<3>(5) (40) -> 95 */
#include "../../runtime/easy68k.h"

constexpr uint32_t fib_ct(uint32_t n) {
  uint32_t a = 0, b = 1;
  for (uint32_t i = 0; i < n; ++i) {
    uint32_t next = a + b;
    a = b;
    b = next;
  }
  return a;
}

int main() {
  constexpr uint32_t kFib10 = fib_ct(10);
  return (int)(kFib10 + (5 << 3));
}
