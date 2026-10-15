/* Solution P-06-EX1 (CC0-1.0) */
#include "../../c/runtime/easy68k.h"

int main(void) {
    int x0 = 20 + 4;
    int x1 = 28 - 2;
    int diff = x0 - x1;
    if (diff < 0) diff = -diff;
    return (diff < 8) ? 1 : 0;
}
