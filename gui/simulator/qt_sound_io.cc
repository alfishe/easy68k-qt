// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "qt_sound_io.h"

#include <algorithm>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSoundEffect>
#include <QUrl>
#include <utility>

#include "gui_thread.h"

namespace easym68k::gui {

namespace {

class EffectVoice : public QtSoundIO::Voice {
 public:
  explicit EffectVoice(const QString& path) { effect_.setSource(QUrl::fromLocalFile(path)); }
  void Play(bool loop) override {
    effect_.setLoopCount(loop ? QSoundEffect::Infinite : 1);
    effect_.play();
    started_ = true;
  }
  void Stop() override {
    effect_.stop();
    started_ = false;
  }
  // Playing, or asked to play while the file still loads.
  bool IsPlaying() const override {
    return effect_.isPlaying() || (started_ && effect_.status() == QSoundEffect::Loading);
  }

 private:
  QSoundEffect effect_;
  bool started_ = false;
};

bool ValidWave(int index) {
  return index >= 0 && index < QtSoundIO::kWaves;
}

}  // namespace

QtSoundIO::VoiceFactory QtSoundIO::EffectVoiceFactory() {
  return [](const QString& path) -> std::unique_ptr<Voice> {
    return std::make_unique<EffectVoice>(path);
  };
}

QtSoundIO::QtSoundIO(QObject* gui_context, VoiceFactory factory)
    : receiver_(std::make_unique<QObject>()), factory_(std::move(factory)) {
  receiver_->moveToThread(gui_context->thread());
}

QtSoundIO::~QtSoundIO() = default;

void QtSoundIO::SetBaseDir(const QString& dir) {
  PostToThread(receiver_.get(), [this, dir] { base_dir_ = dir; });
}

QString QtSoundIO::Resolve(const char* filename) const {
  const QString name = QFile::decodeName(filename);
  if (base_dir_.isEmpty() || QFileInfo(name).isAbsolute())
    return name;
  return QDir(base_dir_).filePath(name);
}

std::unique_ptr<QTemporaryFile> QtSoundIO::ReadIntoMemory(const QString& path) const {
  QFile in(path);
  if (!in.open(QIODevice::ReadOnly))
    return nullptr;
  auto copy =
      std::make_unique<QTemporaryFile>(QDir::tempPath() + QStringLiteral("/easy68k-XXXXXX.wav"));
  if (!copy->open() || copy->write(in.readAll()) < 0)
    return nullptr;
  copy->flush();
  return copy;
}

// ---- Standard player --------------------------------------------------------------

bool QtSoundIO::StandardBusy() const {
  return standard_ && standard_->IsPlaying();
}

// PlaySound(..., SND_ASYNC | SND_NOSTOP [| SND_LOOP]): 0 while another
// sound plays, else 1 (0 as well if the sound cannot be played).
short QtSoundIO::StandardPlay(const QString& path, bool loop) {
  if (StandardBusy())
    return 0;
  standard_ = factory_ ? factory_(path) : nullptr;
  if (!standard_)
    return 0;
  standard_->Play(loop);
  return 1;
}

void QtSoundIO::StandardStop() {  // PlaySound(NULL, NULL, SND_PURGE)
  if (standard_)
    standard_->Stop();
}

// simIOu.cpp playSound(): a missing file leaves the result unchanged.
void QtSoundIO::PlaySound(const char* filename, short* result) {
  const std::string name = filename;
  *result = CallInThread<short>(receiver_.get(), [this, &name, r = *result]() -> short {
    const QString path = Resolve(name.c_str());
    if (!QFileInfo::exists(path))
      return r;
    return StandardPlay(path, false);
  });
}

// simIOu.cpp loadSound(): a missing file leaves the slot as it was.
void QtSoundIO::LoadSound(const char* filename, int wave_index) {
  if (!ValidWave(wave_index))
    return;
  const std::string name = filename;
  CallInThread<bool>(receiver_.get(), [this, &name, wave_index] {
    auto data = ReadIntoMemory(Resolve(name.c_str()));
    if (data)
      standard_waves_[wave_index].file = std::move(data);
    return true;
  });
}

// simIOu.cpp playSoundMem(): nothing loaded leaves the result unchanged.
void QtSoundIO::PlaySoundMem(int wave_index, short* result) {
  if (!ValidWave(wave_index))
    return;
  *result = CallInThread<short>(receiver_.get(), [this, wave_index, r = *result]() -> short {
    const Loaded& wave = standard_waves_[wave_index];
    if (!wave.file)
      return r;
    current_wave_ = wave_index;
    return StandardPlay(wave.file->fileName(), false);
  });
}

// simIOu.cpp controlSound(): 0 play once, 1 loop, 2 stop this sound (if it
// is the current one), 3 stop all; result 0 on error.
void QtSoundIO::ControlSound(int control, int wave_index, short* result) {
  *result = CallInThread<short>(receiver_.get(), [this, control, wave_index]() -> short {
    short r = 0;
    if (ValidWave(wave_index) && standard_waves_[wave_index].file) {
      const QString path = standard_waves_[wave_index].file->fileName();
      switch (control) {
        case 0:
        case 1:
          current_wave_ = wave_index;
          r = StandardPlay(path, control == 1);
          break;
        case 2:
          if (wave_index == current_wave_) {
            StandardStop();
            r = 1;
          }
          break;
        default:
          break;
      }
    }
    if (control == 3) {
      StandardStop();
      r = 1;
    }
    return r;
  });
}

// ---- Multi-voice player -----------------------------------------------------------

// simIOu.cpp playSoundDX(): 1 if the sound plays.
void QtSoundIO::PlaySoundMulti(const char* filename, short* result) {
  const std::string name = filename;
  *result = CallInThread<short>(receiver_.get(), [this, &name]() -> short {
    const QString path = Resolve(name.c_str());
    if (!QFileInfo::exists(path) || !factory_)
      return 0;
    auto voice = factory_(path);
    if (!voice)
      return 0;
    voice->Play(false);
    // Finished sounds are released as new ones start.
    multi_files_.erase(std::remove_if(multi_files_.begin(), multi_files_.end(),
                                      [](const auto& v) { return !v->IsPlaying(); }),
                       multi_files_.end());
    multi_files_.push_back(std::move(voice));
    return 1;
  });
}

// simIOu.cpp loadSoundDX(): replaces the slot; 1 on success. A missing file
// leaves the slot unchanged; a file that cannot be loaded empties it (the
// original kept a pointer to the released segment).
void QtSoundIO::LoadSoundMulti(const char* filename, int wave_index, short* result) {
  if (!ValidWave(wave_index)) {
    *result = 0;
    return;
  }
  const std::string name = filename;
  *result = CallInThread<short>(receiver_.get(), [this, &name, wave_index]() -> short {
    const QString path = Resolve(name.c_str());
    if (!QFileInfo::exists(path))
      return 0;
    Loaded& wave = multi_waves_[wave_index];
    if (wave.voice)
      wave.voice->Stop();
    wave = Loaded{};
    auto data = ReadIntoMemory(path);
    if (!data || !factory_)
      return 0;
    auto voice = factory_(data->fileName());
    if (!voice)
      return 0;
    wave.file = std::move(data);
    wave.voice = std::move(voice);
    return 1;
  });
}

void QtSoundIO::PlaySoundMemMulti(int wave_index, short* result) {
  *result = CallInThread<short>(receiver_.get(), [this, wave_index]() -> short {
    if (!ValidWave(wave_index) || !multi_waves_[wave_index].voice)
      return 0;
    multi_waves_[wave_index].voice->Play(false);
    return 1;
  });
}

void QtSoundIO::StopSoundMemMulti(int wave_index, short* result) {
  *result = CallInThread<short>(receiver_.get(), [this, wave_index]() -> short {
    if (!ValidWave(wave_index) || !multi_waves_[wave_index].voice)
      return 0;
    multi_waves_[wave_index].voice->Stop();
    return 1;
  });
}

void QtSoundIO::StopAllMulti() {
  for (auto& wave : multi_waves_)
    if (wave.voice)
      wave.voice->Stop();
  for (auto& voice : multi_files_)
    voice->Stop();
  multi_files_.clear();
}

// simIOu.cpp controlSoundDX(): 0 play once, 1 loop, 2 stop, 3 stop all
// (here for any index; the original required a loaded one).
void QtSoundIO::ControlSoundMulti(int control, int wave_index, short* result) {
  *result = CallInThread<short>(receiver_.get(), [this, control, wave_index]() -> short {
    if (control == 3) {
      StopAllMulti();
      return 1;
    }
    if (!ValidWave(wave_index) || !multi_waves_[wave_index].voice)
      return 0;
    Voice& voice = *multi_waves_[wave_index].voice;
    switch (control) {
      case 0:
        voice.Play(false);
        return 1;
      case 1:
        voice.Play(true);
        return 1;
      case 2:
        voice.Stop();
        return 1;
      default:
        return 0;
    }
  });
}

// simIOu.cpp ResetSounds() (program start): stop everything, forget loads.
void QtSoundIO::ResetSounds() {
  CallInThread<bool>(receiver_.get(), [this] {
    StandardStop();
    standard_.reset();
    current_wave_ = -1;
    StopAllMulti();
    for (auto& wave : standard_waves_)
      wave = Loaded{};
    for (auto& wave : multi_waves_)
      wave = Loaded{};
    return true;
  });
}

}  // namespace easym68k::gui
