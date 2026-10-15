// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// QtSerialIO (Trap #15 tasks 40-43): CID checks, and on POSIX a real
// exchange through a pseudo-terminal pair (the slave side is opened as the
// serial port).

#include "qt_serial_io.h"

#include <gtest/gtest.h>

#include <cstring>
#include <string>

#if defined(EASYM68K_HAS_SERIALPORT) && !defined(_WIN32)
#include <fcntl.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#define EASY68K_TEST_PTY 1
#endif

namespace easym68k::gui {
namespace {

TEST(QtSerialIOTest, InvalidAndUnopenedPorts) {
  QtSerialIO io;
  short result = -1;
  io.InitPort(16, "COM1", &result);
  EXPECT_EQ(result, 1);
  io.InitPort(-1, "COM1", &result);
  EXPECT_EQ(result, 1);
  io.InitPort(0, "no-such-port-easy68k", &result);
  EXPECT_EQ(result, 2);
  io.SetPortParams(0, 0, &result);
  EXPECT_EQ(result, 3);  // not initialised
  unsigned char n = 4;
  char buf[260];
  io.ReadString(3, &n, buf, &result);
  EXPECT_EQ(result, 3);
  io.SendString(99, &n, "abcd", &result);
  EXPECT_EQ(result, 1);
}

#ifdef EASY68K_TEST_PTY
class PtyTest : public ::testing::Test {
 protected:
  void SetUp() override {
    master_ = posix_openpt(O_RDWR | O_NOCTTY);
    ASSERT_GE(master_, 0);
    ASSERT_EQ(grantpt(master_), 0);
    ASSERT_EQ(unlockpt(master_), 0);
    slave_ = ptsname(master_);
    termios t{};
    tcgetattr(master_, &t);
    cfmakeraw(&t);
    tcsetattr(master_, TCSANOW, &t);
  }
  void TearDown() override {
    if (master_ >= 0)
      close(master_);
  }
  std::string ReadMaster(size_t n) {
    std::string out;
    for (int tries = 0; tries < 200 && out.size() < n; tries++) {
      char c[64];
      const ssize_t got = read(master_, c, sizeof(c));
      if (got > 0)
        out.append(c, static_cast<size_t>(got));
      else
        usleep(5000);
    }
    return out;
  }

  int master_ = -1;
  std::string slave_;
};

TEST_F(PtyTest, SendReadAndTimeouts) {
  fcntl(master_, F_SETFL, fcntl(master_, F_GETFL) | O_NONBLOCK);
  QtSerialIO io;
  short result = -1;
  io.InitPort(2, slave_.c_str(), &result);
  ASSERT_EQ(result, 0);
  io.SetPortParams(2, 12, &result);  // 115200,N,8,1 (a pty accepts only some formats)
  EXPECT_EQ(result, 0);
  io.SetPortParams(2, 0, &result);  // back to 9600,N,8,1
  EXPECT_EQ(result, 0);

  unsigned char n = 5;
  io.SendString(2, &n, "hello", &result);
  EXPECT_EQ(result, 0);
  EXPECT_EQ(n, 5);
  EXPECT_EQ(ReadMaster(5), "hello");

  ASSERT_EQ(write(master_, "abc", 3), 3);
  usleep(20000);
  char buf[260];
  std::memset(buf, 'x', sizeof(buf));
  n = 3;
  io.ReadString(2, &n, buf, &result);
  EXPECT_EQ(result, 0);
  EXPECT_EQ(n, 3);
  EXPECT_STREQ(buf, "abc");

  n = 10;  // nothing more arrives: short read is a timeout
  io.ReadString(2, &n, buf, &result);
  EXPECT_EQ(result, 4);
  EXPECT_EQ(n, 0);
  EXPECT_EQ(buf[0], '\0');

  io.InitPort(2, slave_.c_str(), &result);  // reuse an open CID
  EXPECT_EQ(result, 0);
  io.CloseAll();
  io.SetPortParams(2, 0, &result);
  EXPECT_EQ(result, 3);
}
#endif

}  // namespace
}  // namespace easym68k::gui
