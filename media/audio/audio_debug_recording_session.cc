// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/audio_debug_recording_session.h"
#include "base/bind.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/single_thread_task_runner.h"
#include "media/audio/audio_manager.h"

namespace media {

AudioDebugRecordingSession::~AudioDebugRecordingSession() {}

std::unique_ptr<AudioDebugRecordingSession>
AudioDebugRecordingSession::CreateSession(const base::FilePath& file_path) {
  return std::make_unique<AudioDebugRecordingSessionImpl>(AudioManager::Get(),
                                                          file_path);
}

AudioDebugRecordingSessionImpl::AudioDebugRecordingSessionImpl(
    AudioManager* audio_manager,
    const base::FilePath& file_path)
    : audio_manager_(audio_manager) {
  // This object must outlive AudioManager because it posts unretained.
  audio_manager_->GetTaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&AudioManager::EnableDebugRecording,
                                base::Unretained(audio_manager_), file_path));
}

AudioDebugRecordingSessionImpl::~AudioDebugRecordingSessionImpl() {
  // This object must outlive AudioManager because it posts unretained.
  audio_manager_->GetTaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&AudioManager::DisableDebugRecording,
                                base::Unretained(audio_manager_)));
}

}  // namespace media
