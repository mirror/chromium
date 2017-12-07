// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/audio_debug_recording_manager.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"

namespace media {

namespace {

// Running id recording sources.
int g_next_stream_id = 1;

#if defined(OS_WIN)
#define IntToStringType base::IntToString16
#else
#define IntToStringType base::IntToString
#endif

// Helper function that returns |file_name_extension| and |id| appended to be
// used as debug recording file name suffix.
base::FilePath GetDebugRecordingFileNameSuffix(
    const base::FilePath::StringType& file_name_extension,
    int id) {
  return base::FilePath()
      .AddExtension(file_name_extension)
      .AddExtension(IntToStringType(id))
      .AddExtension(FILE_PATH_LITERAL("wav"));
}

}  // namespace

AudioDebugRecordingManager::AudioDebugRecordingManager(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : task_runner_(std::move(task_runner)), weak_factory_(this) {}

AudioDebugRecordingManager::~AudioDebugRecordingManager() = default;

void AudioDebugRecordingManager::EnableDebugRecording(
    mojom::AudioDebugRecordingFileProviderPtr file_provider) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(!file_provider);

  debug_recording_file_provider_ = std::move(file_provider);

  for (const auto& it : debug_recording_helpers_) {
    debug_recording_file_provider_->GetFileHandle(
        GetDebugRecordingFileNameSuffix(it.second.second, it.first),
        base::BindOnce(&AudioDebugRecordingHelper::EnableDebugRecording,
                       base::Unretained(it.second.first)));
  }
}

void AudioDebugRecordingManager::DisableDebugRecording() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  for (const auto& it : debug_recording_helpers_)
    it.second.first->DisableDebugRecording();
  debug_recording_file_provider_.reset();
}

std::unique_ptr<AudioDebugRecorder>
AudioDebugRecordingManager::RegisterDebugRecordingSource(
    const base::FilePath::StringType& file_name_extension,
    const AudioParameters& params) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  const int id = g_next_stream_id++;

  // Normally, the manager will outlive the one who registers and owns the
  // returned recorder. But to not require this we use a weak pointer.
  std::unique_ptr<AudioDebugRecordingHelper> recording_helper =
      CreateAudioDebugRecordingHelper(
          params, task_runner_,
          base::BindOnce(
              &AudioDebugRecordingManager::UnregisterDebugRecordingSource,
              weak_factory_.GetWeakPtr(), id));

  if (IsDebugRecordingEnabled()) {
    debug_recording_file_provider_->GetFileHandle(
        GetDebugRecordingFileNameSuffix(file_name_extension, id),
        base::BindOnce(&AudioDebugRecordingHelper::EnableDebugRecording,
                       base::Unretained(recording_helper.get())));
  }

  debug_recording_helpers_[id] =
      std::make_pair(recording_helper.get(), file_name_extension);

  return base::WrapUnique<AudioDebugRecorder>(recording_helper.release());
}

void AudioDebugRecordingManager::UnregisterDebugRecordingSource(int id) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  auto it = debug_recording_helpers_.find(id);
  DCHECK(it != debug_recording_helpers_.end());
  debug_recording_helpers_.erase(id);
}

std::unique_ptr<AudioDebugRecordingHelper>
AudioDebugRecordingManager::CreateAudioDebugRecordingHelper(
    const AudioParameters& params,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    base::OnceClosure on_destruction_closure) {
  return base::MakeUnique<AudioDebugRecordingHelper>(
      params, task_runner, std::move(on_destruction_closure));
}

bool AudioDebugRecordingManager::IsDebugRecordingEnabled() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  return !debug_recording_file_provider_;
}

}  // namespace media
