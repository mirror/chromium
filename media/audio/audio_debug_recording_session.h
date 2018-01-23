// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_AUDIO_DEBUG_RECORDING_SESSION_H_
#define MEDIA_AUDIO_AUDIO_DEBUG_RECORDING_SESSION_H_

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "media/base/media_export.h"

namespace base {

class File;

}  // namespace base

namespace media {

class AudioManager;

// Creating an AudioDebugRecordingSession object will enable audio debug
// recording. This object posts |audio_manager_| unretained on the audio thread,
// on which AudioManager lives, therefore AudioManager must outlive this object.
class MEDIA_EXPORT AudioDebugRecordingSession {
 public:
  AudioDebugRecordingSession(AudioManager*, const base::FilePath);
  ~AudioDebugRecordingSession();

 private:
  void CreateFile(const base::FilePath& file_name,
                  base::OnceCallback<void(base::File)> reply_callback);
  AudioManager* audio_manager_;
  base::FilePath base_file_path_;
  base::WeakPtrFactory<AudioDebugRecordingSession> weak_factory_;
};

}  // namespace media

#endif  // MEDIA_AUDIO_AUDIO_DEBUG_RECORDING_SESSION_H_
