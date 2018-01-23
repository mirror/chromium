// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/audio_debug_recording_session.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "media/audio/audio_manager.h"

namespace media {

AudioDebugRecordingSession::AudioDebugRecordingSession(
    AudioManager* audio_manager,
    const base::FilePath file_path)
    : audio_manager_(audio_manager),
      base_file_path_(file_path),
      weak_factory_(this) {
  // This object must outlive AudioManager because it posts unretained.
  audio_manager_->GetTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &AudioManager::EnableDebugRecording, base::Unretained(audio_manager_),
          base::BindRepeating(&AudioDebugRecordingSession::CreateFile,
                              weak_factory_.GetWeakPtr())));
}

AudioDebugRecordingSession::~AudioDebugRecordingSession() {
  // This object must outlive AudioManager because it posts unretained.
  audio_manager_->GetTaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&AudioManager::DisableDebugRecording,
                                base::Unretained(audio_manager_)));
}

void AudioDebugRecordingSession::CreateFile(
    const base::FilePath& suffix,
    base::OnceCallback<void(base::File)> reply_callback) {
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BACKGROUND,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(
          [](const base::FilePath& file_name) {
            return base::File(file_name, base::File::FLAG_CREATE_ALWAYS |
                                             base::File::FLAG_WRITE);
          },
          base_file_path_.InsertBeforeExtension(suffix.value())),
      std::move(reply_callback));
}

}  // namespace media
