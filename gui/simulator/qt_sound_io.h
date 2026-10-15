// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// ISoundIO for Sim68K-Qt (Trap #15 tasks 70-77), after simIOu.cpp:
//  - the standard player (Win32 PlaySound, tasks 70-72, 76): one sound at a
//    time; a request while a sound plays is refused (SND_NOSTOP, result 0);
//    256 sounds loaded into memory;
//  - the multi-voice player (DirectX, tasks 73-75, 77): sounds play
//    simultaneously; 256 loaded sounds.
// Sounds play on the GUI thread; the simulator thread waits for the result.
// Loading reads the file at once, so it may change or vanish afterwards.

#ifndef EASY68K_GUI_SIMULATOR_QT_SOUND_IO_H_
#define EASY68K_GUI_SIMULATOR_QT_SOUND_IO_H_

#include <array>
#include <functional>
#include <memory>
#include <QObject>
#include <QString>
#include <QTemporaryFile>
#include <vector>

#include "easym68k/sim/isound_io.h"

namespace easym68k::gui {

class QtSoundIO : public sim::ISoundIO {
 public:
  static constexpr int kWaves = 256;

  // One playable sound (GUI thread).
  class Voice {
   public:
    virtual ~Voice() = default;
    virtual void Play(bool loop) = 0;
    virtual void Stop() = 0;
    virtual bool IsPlaying() const = 0;
  };
  // Creates a voice for a WAV file (GUI thread); null if it cannot play.
  using VoiceFactory = std::function<std::unique_ptr<Voice>(const QString& path)>;
  // Voices played with QSoundEffect (Qt Multimedia).
  static VoiceFactory EffectVoiceFactory();

  QtSoundIO(QObject* gui_context, VoiceFactory factory);
  ~QtSoundIO() override;

  // Directory for relative file names (the loaded program's).
  void SetBaseDir(const QString& dir);

  void PlaySound(const char* filename, short* result) override;
  void LoadSound(const char* filename, int wave_index) override;
  void PlaySoundMem(int wave_index, short* result) override;
  void ControlSound(int control, int wave_index, short* result) override;
  void PlaySoundMulti(const char* filename, short* result) override;
  void LoadSoundMulti(const char* filename, int wave_index, short* result) override;
  void PlaySoundMemMulti(int wave_index, short* result) override;
  void StopSoundMemMulti(int wave_index, short* result) override;
  void ControlSoundMulti(int control, int wave_index, short* result) override;
  void ResetSounds() override;

 private:
  struct Loaded {  // a sound read into memory
    std::unique_ptr<QTemporaryFile> file;
    std::unique_ptr<Voice> voice;  // multi-voice player only
  };

  // GUI thread only.
  QString Resolve(const char* filename) const;
  std::unique_ptr<QTemporaryFile> ReadIntoMemory(const QString& path) const;
  bool StandardBusy() const;
  short StandardPlay(const QString& path, bool loop);
  void StandardStop();
  void StopAllMulti();

  std::unique_ptr<QObject> receiver_;  // GUI-thread context
  VoiceFactory factory_;
  QString base_dir_;
  std::unique_ptr<Voice> standard_;  // the standard player's sound
  int current_wave_ = -1;
  std::array<Loaded, kWaves> standard_waves_;
  std::array<Loaded, kWaves> multi_waves_;
  std::vector<std::unique_ptr<Voice>> multi_files_;  // task 73 sounds
};

}  // namespace easym68k::gui

#endif  // EASY68K_GUI_SIMULATOR_QT_SOUND_IO_H_
