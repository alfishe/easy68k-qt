// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// ISoundIO — audio playback interface (Trap #15 tasks 70-77).
// Unifies single-voice (replaces VCL TMediaPlayer) and multi-voice
// (replaces DirectX DirectMusic) playback in original CODE9.CPP.

#pragma once

namespace easym68k::sim {

class ISoundIO {
 public:
  virtual ~ISoundIO() = default;

  // Single-voice player (tasks 70-73)
  virtual void PlaySound(const char* filename, short* result) = 0;
  virtual void LoadSound(const char* filename, int wave_index) = 0;
  virtual void PlaySoundMem(int wave_index, short* result) = 0;
  virtual void ControlSound(int control, int wave_index, short* result) = 0;

  // Multi-voice player (tasks 74-77)
  virtual void PlaySoundMulti(const char* filename, short* result) = 0;
  virtual void LoadSoundMulti(const char* filename, int wave_index, short* result) = 0;
  virtual void PlaySoundMemMulti(int wave_index, short* result) = 0;
  virtual void StopSoundMemMulti(int wave_index, short* result) = 0;
  virtual void ControlSoundMulti(int control, int wave_index, short* result) = 0;

  // Lifecycle
  virtual void ResetSounds() = 0;
};

}  // namespace easym68k::sim
