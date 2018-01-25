// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_AUDIO_DEBUG_RECORDING_SESSION_H_
#define MEDIA_AUDIO_AUDIO_DEBUG_RECORDING_SESSION_H_

#include <memory>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "media/base/media_export.h"

namespace base {
class File;
class FilePath;
}  // namespace base

namespace media {

class AudioManager;

class MEDIA_EXPORT AudioDebugRecordingSession {
 public:
  using CreateFileCallback = base::RepeatingCallback<void(
      const base::FilePath&,
      base::OnceCallback<void(base::File)> reply_callback)>;

  virtual ~AudioDebugRecordingSession();
  static std::unique_ptr<AudioDebugRecordingSession> CreateSession(
      const base::FilePath&);
};

// Creating/Destroying an AudioDebugRecordingSessionImpl object enables/disables
// audio debug recording. This object posts |audio_manager_| unretained on the
// audio thread, on which AudioManager lives, therefore AudioManager must
// outlive this object.
class MEDIA_EXPORT AudioDebugRecordingSessionImpl
    : public AudioDebugRecordingSession {
 public:
  AudioDebugRecordingSessionImpl(AudioManager* audio_manager,
                                 const base::FilePath&);
  ~AudioDebugRecordingSessionImpl() override;

 private:
  AudioManager* audio_manager_;
};
}  // namespace media

#endif  // MEDIA_AUDIO_AUDIO_DEBUG_RECORDING_SESSION_H_
