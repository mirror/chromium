// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "media/mojo/services/mojo_audio_debug_recording.h"
#include "media/audio/audio_manager_base.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace media {

MojoAudioDebugRecording::MojoAudioDebugRecording(
    mojom::AudioDebugRecordingRequest request,
    AudioManager* audio_manager)
    : binding_(this, std::move(request)), audio_manager_(audio_manager) {}

MojoAudioDebugRecording::~MojoAudioDebugRecording() {
  audio_manager_->DisableDebugRecording();
}

void MojoAudioDebugRecording::EnableDebugRecording(
    mojom::AudioDebugRecordingFileProviderPtr file_provider) {
  DCHECK(audio_manager_);
  audio_manager_->EnableDebugRecording(std::move(file_provider));
}
}  // namespace media
