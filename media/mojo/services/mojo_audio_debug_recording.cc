// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "media/mojo/services/mojo_audio_debug_recording.h"
#include "media/audio/audio_debug_recording_manager.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace media {

MojoDebugRecording::MojoDebugRecording(
    mojom::DebugRecordingRequest request,
    AudioDebugRecordingManager* recording_manager)
    : binding_(this, std::move(request)),
      recording_manager_(recording_manager) {}

MojoDebugRecording::~MojoDebugRecording() {
  recording_manager_->DisableDebugRecording();
}

void MojoDebugRecording::Enable(
    mojom::DebugRecordingFileProviderPtr file_provider) {
  DCHECK(recording_manager_);
  // TODO bind file creation callback to file provider
  // recording_manager_->EnableDebugRecording();
}
}  // namespace media
