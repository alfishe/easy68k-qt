// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// StdFileIO (Trap #15 file tasks 50-59, SIMOPS2.CPP port) and the file task
// dispatch (Task 10.4). The common paths are compared with the original by
// the golden trace `trap15_file`; these tests cover the original bugs the
// port fixes and the checks done before the file calls.

#include "easym68k/sim/std_file_io.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>

#include "easym68k/sim/simulator.h"

namespace easym68k::sim {
namespace {

namespace fs = std::filesystem;

class StdFileIOTest : public ::testing::Test {
 protected:
  void SetUp() override {
    dir_ = fs::temp_directory_path() /
           ("easy68k_fileio_" + std::to_string(reinterpret_cast<uintptr_t>(this)));
    fs::remove_all(dir_);
    fs::create_directories(dir_);
  }
  void TearDown() override { fs::remove_all(dir_); }

  fs::path dir_;
};

TEST_F(StdFileIOTest, WriteReadPositionClose) {
  StdFileIO io(dir_.string());
  long fid = 0;
  short r = -1;
  io.NewFile(&fid, "a.bin", &r);
  ASSERT_EQ(r, kFileSuccess);
  EXPECT_EQ(fid, 0);
  io.WriteFile(fid, "hello world", 11, &r);
  EXPECT_EQ(r, kFileSuccess);
  io.PositionFile(fid, 6, &r);
  EXPECT_EQ(r, kFileSuccess);
  char buf[16] = {};
  unsigned int size = 10;
  io.ReadFile(fid, buf, &size, &r);
  EXPECT_EQ(r, kFileSuccess);  // partial read is a success
  EXPECT_EQ(size, 5u);
  EXPECT_STREQ(buf, "world");
  io.CloseFile(fid, &r);
  EXPECT_EQ(r, kFileSuccess);
  io.CloseFile(fid, &r);
  EXPECT_EQ(r, kFileError);
  EXPECT_TRUE(fs::exists(dir_ / "a.bin"));
}

// Original bug: at EOF readFile() left the size at the requested count.
TEST_F(StdFileIOTest, ReadAtEofReportsZeroBytes) {
  StdFileIO io(dir_.string());
  long fid;
  short r;
  io.NewFile(&fid, "e.bin", &r);
  char buf[8];
  unsigned int size = 8;
  io.ReadFile(fid, buf, &size, &r);
  EXPECT_EQ(r, kFileEof);
  EXPECT_EQ(size, 0u);
}

// Original bug: a zero-length write was reported as an error.
TEST_F(StdFileIOTest, ZeroLengthWriteSucceeds) {
  StdFileIO io(dir_.string());
  long fid;
  short r;
  io.NewFile(&fid, "z.bin", &r);
  io.WriteFile(fid, "", 0, &r);
  EXPECT_EQ(r, kFileSuccess);
}

// Original bug: fileOp() called fclose(NULL) for a missing file.
TEST_F(StdFileIOTest, FileOpOnMissingFile) {
  StdFileIO io(dir_.string());
  long mode = 0;
  short r = -1;
  io.FileOp(&mode, "missing.bin", &r);
  EXPECT_EQ(r, kFileError);
}

TEST_F(StdFileIOTest, OpenMissingAndTableLimit) {
  StdFileIO io(dir_.string());
  long fid = 0;
  short r;
  io.OpenFile(&fid, "missing.bin", &r);
  EXPECT_EQ(r, kFileError);
  EXPECT_EQ(fid, -1);
  for (int i = 0; i < StdFileIO::kMaxFiles; i++) {
    io.NewFile(&fid, ("f" + std::to_string(i)).c_str(), &r);
    EXPECT_EQ(fid, i);
  }
  io.NewFile(&fid, "one_too_many", &r);
  EXPECT_EQ(r, kFileError);
  EXPECT_EQ(fid, -1);
  io.CloseAll(&r);
  EXPECT_EQ(r, kFileSuccess);
  io.DeleteFile("f0", &r);
  EXPECT_EQ(r, kFileSuccess);
  io.DeleteFile("f0", &r);
  EXPECT_EQ(r, kFileError);
}

// ---------------------------------------------------------------------------
// Dispatch
// ---------------------------------------------------------------------------

class FileTaskTest : public StdFileIOTest {
 protected:
  void Run(std::initializer_list<uint16_t> words) {
    io_ = std::make_unique<StdFileIO>(dir_.string());
    M68kSimulator::Config config;
    config.file_io = io_.get();
    sim_ = std::make_unique<M68kSimulator>(config);
    sim_->GetMemory().Erase();
    uint32_t a = 0x1000;
    for (uint16_t w : words) {
      sim_->GetMemory().WriteWord(a, w);
      a += 2;
    }
    sim_->SetStartAddress(0x1000);
    sim_->InitProgram();
  }
  std::unique_ptr<StdFileIO> io_;
  std::unique_ptr<M68kSimulator> sim_;
};

// A buffer reaching past the end of the 68000 memory: F_ERROR, no access.
TEST_F(FileTaskTest, BufferBeyondMemoryIsAnError) {
  Run({0x4E4F});            // TRAP #15
  sim_->State().d[0] = 54;  // write
  sim_->State().d[1] = 0;
  sim_->State().d[2] = 0x20;
  sim_->State().a[1] = 0xFFFFF0;
  EXPECT_EQ(sim_->Step(), SimResult::kOk);
  EXPECT_EQ(sim_->State().d[0] & 0xFFFF, static_cast<uint32_t>(kFileError));
}

// The file name check raises a bus error for the Invalid memory map.
TEST_F(FileTaskTest, InvalidMapNameIsBusError) {
  Run({0x4E4F});
  sim_->GetMemory().WriteLong(0x08, 0x3000);  // bus error vector
  M68kSimulator::MemoryMapConfig map;
  map.invalid = {true, 0x5000, 0x5FFF};
  sim_->ConfigureMemoryMap(map);
  std::strcpy(reinterpret_cast<char*>(sim_->GetMemory().RawData() + 0x5000), "x.bin");
  sim_->State().d[0] = 52;
  sim_->State().a[1] = 0x5000;
  sim_->State().ssp = 0x8000;
  EXPECT_EQ(sim_->Step(), SimResult::kBusError);
  EXPECT_EQ(sim_->State().pc, 0x3000u);
  EXPECT_FALSE(fs::exists(dir_ / "x.bin"));
}

// Task 59 passes D1 as the mode; D2 must not be touched (a long* aliasing
// D1 used to write 8 bytes on 64-bit hosts).
TEST_F(FileTaskTest, FileOpKeepsOtherRegisters) {
  Run({0x4E4F});
  std::strcpy(reinterpret_cast<char*>(sim_->GetMemory().RawData() + 0x2000), "none");
  sim_->State().d[0] = 59;
  sim_->State().d[1] = 0;
  sim_->State().d[2] = 0x12345678;
  sim_->State().a[1] = 0x2000;
  sim_->Step();
  EXPECT_EQ(sim_->State().d[2], 0x12345678u);
  EXPECT_EQ(sim_->State().d[0] & 0xFFFF, static_cast<uint32_t>(kFileError));
}

}  // namespace
}  // namespace easym68k::sim
