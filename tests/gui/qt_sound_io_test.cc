// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// QtSoundIO (Trap #15 tasks 70-77) with recording fake voices: the standard
// player's one-at-a-time rule, the 256 loaded sounds, the multi-voice
// player and the results; plus a smoke test of the QSoundEffect voices.

#include "qt_sound_io.h"

#include <gtest/gtest.h>

#include <atomic>
#include <map>
#include <memory>
#include <QCoreApplication>
#include <QDataStream>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryDir>
#include <thread>

namespace easym68k::gui {
namespace {

// A fake voice: "plays" until stopped (or, if not looping, until Finish()).
struct VoiceLog {
  int created = 0;
  int plays = 0;
  int loops = 0;
  int stops = 0;
};

class FakeVoice : public QtSoundIO::Voice {
 public:
  explicit FakeVoice(VoiceLog* log) : log_(log) { log_->created++; }
  void Play(bool loop) override {
    playing_ = true;
    (loop ? log_->loops : log_->plays)++;
  }
  void Stop() override {
    if (playing_)
      log_->stops++;
    playing_ = false;
  }
  bool IsPlaying() const override { return playing_ && !finished_; }
  void Finish() { finished_ = true; }

 private:
  VoiceLog* log_;
  bool playing_ = false;
  bool finished_ = false;
};

class QtSoundIOTest : public ::testing::Test {
 protected:
  QtSoundIOTest() {
    wav_ = dir_.filePath(QStringLiteral("beep.wav"));
    QFile f(wav_);
    f.open(QIODevice::WriteOnly);
    f.write("RIFF....WAVE");
  }

  QtSoundIO::VoiceFactory Factory() {
    return [this](const QString& path) -> std::unique_ptr<QtSoundIO::Voice> {
      last_path_ = path;
      auto v = std::make_unique<FakeVoice>(&log_);
      last_ = v.get();
      return v;
    };
  }

  QTemporaryDir dir_;
  QString wav_;
  QString last_path_;
  VoiceLog log_;
  FakeVoice* last_ = nullptr;
  QObject gui_;
};

TEST_F(QtSoundIOTest, StandardPlayerPlaysOneSoundAtATime) {
  QtSoundIO io(&gui_, Factory());
  io.SetBaseDir(dir_.path());
  short result = 77;
  io.PlaySound("missing.wav", &result);
  EXPECT_EQ(result, 77);              // not found: unchanged
  io.PlaySound("beep.wav", &result);  // relative to the base directory
  EXPECT_EQ(result, 1);
  EXPECT_EQ(log_.plays, 1);
  io.PlaySound(wav_.toLocal8Bit().constData(), &result);
  EXPECT_EQ(result, 0);  // busy (SND_NOSTOP)
  last_->Finish();
  io.PlaySound(wav_.toLocal8Bit().constData(), &result);
  EXPECT_EQ(result, 1);
}

TEST_F(QtSoundIOTest, StandardPlayerMemorySounds) {
  QtSoundIO io(&gui_, Factory());
  short result = 55;
  io.PlaySoundMem(3, &result);
  EXPECT_EQ(result, 55);  // nothing loaded: unchanged
  io.LoadSound(wav_.toLocal8Bit().constData(), 3);
  QFile::remove(wav_);  // the sound was read into memory
  io.PlaySoundMem(3, &result);
  EXPECT_EQ(result, 1);
  EXPECT_NE(last_path_, wav_);
  EXPECT_TRUE(QFile::exists(last_path_));
  io.ControlSound(2, 4, &result);  // not loaded
  EXPECT_EQ(result, 0);
  io.ControlSound(2, 3, &result);  // stop the current sound
  EXPECT_EQ(result, 1);
  EXPECT_EQ(log_.stops, 1);
  io.ControlSound(1, 3, &result);  // loop
  EXPECT_EQ(result, 1);
  EXPECT_EQ(log_.loops, 1);
  io.ControlSound(0, 3, &result);  // busy looping
  EXPECT_EQ(result, 0);
  io.ControlSound(3, 200, &result);  // stop all, any index
  EXPECT_EQ(result, 1);
  io.ControlSound(9, 3, &result);  // reserved
  EXPECT_EQ(result, 0);
}

TEST_F(QtSoundIOTest, MultiVoicePlayer) {
  QtSoundIO io(&gui_, Factory());
  const QByteArray wav = wav_.toLocal8Bit();
  short result = -1;
  io.PlaySoundMulti("missing.wav", &result);
  EXPECT_EQ(result, 0);
  io.PlaySoundMulti(wav.constData(), &result);
  EXPECT_EQ(result, 1);
  io.PlaySoundMulti(wav.constData(), &result);  // simultaneous
  EXPECT_EQ(result, 1);
  EXPECT_EQ(log_.plays, 2);
  io.LoadSoundMulti(wav.constData(), 7, &result);
  EXPECT_EQ(result, 1);
  io.PlaySoundMemMulti(7, &result);
  EXPECT_EQ(result, 1);
  io.PlaySoundMemMulti(8, &result);
  EXPECT_EQ(result, 0);
  io.ControlSoundMulti(1, 7, &result);
  EXPECT_EQ(result, 1);
  EXPECT_EQ(log_.loops, 1);
  io.StopSoundMemMulti(7, &result);
  EXPECT_EQ(result, 1);
  io.ControlSoundMulti(2, 8, &result);  // not loaded
  EXPECT_EQ(result, 0);
  const int stops = log_.stops;
  io.ControlSoundMulti(3, 8, &result);  // stop all, even with an empty index
  EXPECT_EQ(result, 1);
  EXPECT_EQ(log_.stops, stops + 2);  // the two task 73 sounds
  io.LoadSoundMulti("missing.wav", 7, &result);
  EXPECT_EQ(result, 0);
  io.PlaySoundMemMulti(7, &result);  // a missing file leaves the slot
  EXPECT_EQ(result, 1);
}

TEST_F(QtSoundIOTest, ResetForgetsEverything) {
  QtSoundIO io(&gui_, Factory());
  const QByteArray wav = wav_.toLocal8Bit();
  short result = 0;
  io.LoadSound(wav.constData(), 1);
  io.LoadSoundMulti(wav.constData(), 2, &result);
  io.PlaySoundMem(1, &result);
  io.ResetSounds();
  result = 9;
  io.PlaySoundMem(1, &result);
  EXPECT_EQ(result, 9);
  io.PlaySoundMemMulti(2, &result);
  EXPECT_EQ(result, 0);
}

TEST_F(QtSoundIOTest, CallsFromTheSimulatorThreadRunOnTheGuiThread) {
  std::thread::id voice_thread;
  QtSoundIO io(&gui_, [&](const QString&) -> std::unique_ptr<QtSoundIO::Voice> {
    voice_thread = std::this_thread::get_id();
    return std::make_unique<FakeVoice>(&log_);
  });
  short result = 0;
  std::atomic<bool> done{false};
  const QByteArray wav = wav_.toLocal8Bit();
  std::thread sim([&] {
    io.PlaySoundMulti(wav.constData(), &result);
    done = true;
  });
  QElapsedTimer t;
  t.start();
  while (!done && t.elapsed() < 3000)
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
  sim.join();
  EXPECT_EQ(result, 1);
  EXPECT_EQ(voice_thread, std::this_thread::get_id());
}

// QSoundEffect voices work without an audio device (headless).
TEST_F(QtSoundIOTest, EffectVoicesSmoke) {
  const QString path = dir_.filePath(QStringLiteral("tone.wav"));
  {
    QFile f(path);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    QDataStream out(&f);
    out.setByteOrder(QDataStream::LittleEndian);
    const quint32 samples = 800;  // 0.1 s, 8 kHz, 8-bit mono
    f.write("RIFF");
    out << quint32(36 + samples);
    f.write("WAVEfmt ");
    out << quint32(16) << quint16(1) << quint16(1) << quint32(8000) << quint32(8000) << quint16(1)
        << quint16(8);
    f.write("data");
    out << quint32(samples);
    for (quint32 i = 0; i < samples; i++)
      out << quint8((i / 10) % 2 ? 200 : 56);
  }
  QtSoundIO io(&gui_, QtSoundIO::EffectVoiceFactory());
  const QByteArray name = path.toLocal8Bit();
  short result = 0;
  io.PlaySoundMulti(name.constData(), &result);
  EXPECT_EQ(result, 1);
  io.LoadSoundMulti(name.constData(), 0, &result);
  io.ControlSoundMulti(1, 0, &result);
  io.ControlSoundMulti(3, 0, &result);
  io.PlaySound(name.constData(), &result);
  io.ControlSound(3, 0, &result);
  io.ResetSounds();
  QCoreApplication::processEvents();
}

}  // namespace
}  // namespace easym68k::gui
