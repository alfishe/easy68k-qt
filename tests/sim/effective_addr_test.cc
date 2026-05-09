#include "easym68k/sim/effective_addr.h"

#include <gtest/gtest.h>

namespace easym68k::sim {

TEST(EffectiveAddrTest, MakeDataRegEa) {
  for (int r = 0; r < 8; ++r) {
    EffectiveAddr ea = MakeDataRegEa(r);
    EXPECT_EQ(ea.target, EaTarget::kDataReg);
    EXPECT_EQ(ea.index, static_cast<uint8_t>(r));
    EXPECT_EQ(ea.mode, 0);
    EXPECT_EQ(ea.reg, static_cast<uint8_t>(r));
  }
}

TEST(EffectiveAddrTest, MakeAddrRegEa) {
  for (int r = 0; r < 8; ++r) {
    EffectiveAddr ea = MakeAddrRegEa(r);
    EXPECT_EQ(ea.target, EaTarget::kAddrReg);
    EXPECT_EQ(ea.index, static_cast<uint8_t>(r));
    EXPECT_EQ(ea.mode, 1);
    EXPECT_EQ(ea.reg, static_cast<uint8_t>(r));
  }
}

TEST(EffectiveAddrTest, MakeMemoryEa) {
  EffectiveAddr ea = MakeMemoryEa(0x001000, 5, 3);
  EXPECT_EQ(ea.target, EaTarget::kMemory);
  EXPECT_EQ(ea.address, 0x001000u);
  EXPECT_EQ(ea.mode, 5);
  EXPECT_EQ(ea.reg, 3);
}

TEST(EffectiveAddrTest, MakeImmediateEa) {
  EffectiveAddr ea = MakeImmediateEa(0xDEADBEEF);
  EXPECT_EQ(ea.target, EaTarget::kImmediate);
  EXPECT_EQ(ea.address, 0xDEADBEEFu);
  EXPECT_EQ(ea.mode, 7);
  EXPECT_EQ(ea.reg, 4);
}

TEST(EffectiveAddrTest, DataRegIsNotAddrReg) {
  EffectiveAddr d = MakeDataRegEa(0);
  EffectiveAddr a = MakeAddrRegEa(0);
  EXPECT_NE(d.target, a.target);
}

}  // namespace easym68k::sim
