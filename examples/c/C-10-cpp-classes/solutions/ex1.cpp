/* Solution C-10-EX1: Counter::add with explicit this pointer -> 142 */
#include "../../runtime/easy68k.h"

class Counter {
 public:
  explicit Counter(int32_t init) : value_(init) {}
  void add(int32_t delta) { value_ += delta; }
  int32_t value() const { return value_; }
 private:
  int32_t value_;
};

int main() {
  Counter c(100);
  c.add(30);
  c.add(12);
  return (int)c.value();
}
