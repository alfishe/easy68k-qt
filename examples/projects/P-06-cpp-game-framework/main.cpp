/* P-06: Freestanding C++17 Game Framework with ASM Hot Path (CC0-1.0) */
#include "../c/runtime/easy68k.h"

struct Entity {
    int16_t x;
    int16_t vx;
};

template <int N>
class EntityWorld {
public:
    Entity entities[N];

    void step_all() noexcept {
        for (int i = 0; i < N; ++i) {
            entities[i].x = static_cast<int16_t>(entities[i].x + entities[i].vx);
        }
    }

    static bool aabb_overlap_1d(int16_t x0, int16_t x1, int16_t width) noexcept {
        int16_t diff = static_cast<int16_t>(x0 - x1);
        if (diff < 0) diff = static_cast<int16_t>(-diff);
        return diff < width;
    }
};

int main(void) {
    EntityWorld<2> world{{{40, 5}, {64, -1}}};
    for (int tick = 0; tick < 4; ++tick) {
        world.step_all();
    }
    int hits = EntityWorld<2>::aabb_overlap_1d(
        world.entities[0].x, world.entities[1].x, 8) ? 1 : 0;
    e68k_print("Tick 4: x0=");
    e68k_print_int(world.entities[0].x);
    e68k_print(" hits=");
    e68k_print_int(hits);
    e68k_println("");
    return world.entities[0].x + hits;
}
