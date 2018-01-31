// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/audio_debug_recording_session_impl.h"

#include <utility>

#include "base/bind.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/single_thread_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "media/audio/audio_debug_recording_manager.h"
#include "media/audio/audio_manager.h"

namespace media {

// Posting AudioManager::Get()->GetAudioDebugRecordingManager() as unretained is
// safe because AudioManager::Shutdown() (called before AudioManager
// destruction, that also destroys its member AudioDebugRecordingManager) shuts
// down AudioThread, on which both AudioManager and AudioDebugRecordingManager
// live.

namespace {

void CreateFile(const base::FilePath& debug_recording_file_path,
                const base::FilePath& extension,
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
          debug_recording_file_path.AddExtension(extension.value())),
      std::move(reply_callback));
}

}  // namespace

AudioDebugRecordingSessionImpl::AudioDebugRecordingSessionImpl(
    const base::FilePath& debug_recording_file_path) {
  AudioManager* audio_manager = AudioManager::Get();
  if (audio_manager == nullptr)
    return;

  AudioDebugRecordingManager* debug_recording_manager =
      audio_manager->GetAudioDebugRecordingManager();
  if (debug_recording_manager == nullptr)
    return;

  debug_recording_manager->GetTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &AudioDebugRecordingManager::EnableDebugRecording,
          base::Unretained(debug_recording_manager),
          base::BindRepeating(&CreateFile, debug_recording_file_path)));
}

AudioDebugRecordingSessionImpl::~AudioDebugRecordingSessionImpl() {
  AudioManager* audio_manager = AudioManager::Get();
  if (audio_manager == nullptr)
    return;

  AudioDebugRecordingManager* debug_recording_manager =
      audio_manager->GetAudioDebugRecordingManager();
  if (debug_recording_manager == nullptr)
    return;

  debug_recording_manager->GetTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(&AudioDebugRecordingManager::DisableDebugRecording,
                     base::Unretained(debug_recording_manager)));
}

}  // namespace media
