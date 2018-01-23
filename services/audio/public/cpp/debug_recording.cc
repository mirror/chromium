// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "services/audio/public/cpp/debug_recording.h"
#include "media/audio/audio_manager.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace audio {

DebugRecording::DebugRecording(mojom::DebugRecordingRequest request,
                               media::AudioManager* audio_manager)
    : binding_(this, std::move(request)), audio_manager_(audio_manager) {}

DebugRecording::~DebugRecording() {
  audio_manager_->DisableDebugRecording();
}

void DebugRecording::CreateFile(
    const base::FilePath& file_suffix,
    mojom::DebugRecordingFileProvider::CreateCallback reply_callback) {
  DCHECK(file_provider_);
  file_provider_->Create(file_suffix, std::move(reply_callback));
}

void DebugRecording::Enable(
    mojom::DebugRecordingFileProviderPtr file_provider) {
  DCHECK(audio_manager_);
  file_provider_ = std::move(file_provider);
  // TODO reason about Unretained being safe
  audio_manager_->EnableDebugRecording(
      base::BindRepeating(&DebugRecording::CreateFile, base::Unretained(this)));
}

}  // namespace audio
