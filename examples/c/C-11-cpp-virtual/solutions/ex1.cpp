/* Solution C-11-EX1: Dispatch slot 0 of vtable at 0(A0) -> 30 */
#include "../../runtime/easy68k.h"

struct Node {
  virtual int32_t eval() const { return val * 3; }
  int32_t val;
};

int main() {
  Node n;
  n.val = 10;
  const Node* p = &n;
  return (int)p->eval();
}
