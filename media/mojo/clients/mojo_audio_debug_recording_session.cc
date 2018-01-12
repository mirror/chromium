// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "media/mojo/clients/mojo_audio_debug_recording_session.h"
#include "media/mojo/services/mojo_audio_debug_recording_file_provider.h"

namespace media {

DebugRecordingSession::DebugRecordingSession(
    const base::FilePath& file_name_base) {
  mojom::DebugRecordingFileProviderPtr file_provider_ptr;
  file_provider_impl_ = base::MakeUnique<MojoDebugRecordingFileProvider>(
      mojo::MakeRequest(&file_provider_ptr), file_name_base);
  // audio service will only allow one interface bound at a time
  // TODO obtain connector and bind:
  // connector->BindInterface(kAudioServiceName, &recorder);
  // TODO if binding fails might be signaled through a callback - check
  if (recorder)
    recorder->EnableDebugRecording(std::move(file_provider_ptr));

  return recorder;
}

DebugRecordingSession::~DebugRecordingSession() {
  debug_recorder_.reset();  //
}

}  // namespace media
