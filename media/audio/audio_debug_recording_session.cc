// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/audio_debug_recording_session.h"

#include "base/bind.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/single_thread_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "media/audio/audio_debug_recording_session_impl.h"

namespace base {
class FilePath;
}
namespace media {

AudioDebugRecordingSession::~AudioDebugRecordingSession() {}

std::unique_ptr<AudioDebugRecordingSession> AudioDebugRecordingSession::Create(
    const base::FilePath& debug_recording_file_path) {
  return std::make_unique<AudioDebugRecordingSessionImpl>(
      debug_recording_file_path);
}

}  // namespace media
