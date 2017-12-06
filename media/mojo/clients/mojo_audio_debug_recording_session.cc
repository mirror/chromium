// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "media/mojo/clients/mojo_audio_debug_recording_session.h"

namespace media {

mojom::AudioDebugRecordingPtr AudioDebugRecordingSession::Create(
    mojom::AudioDebugRecordingFileProviderPtr file_provider) {
  mojom::AudioDebugRecordingPtr
      recorder;  // = audio_manager->TryInitalizeDebugRecorder();
  // TODO obtain connector and bind:
  // connector->BindInterface(kAudioServiceName, &recorder);

  // audio service will only allow one interface bound at a time
  // TODO if binding fails might be signaled through a callback - check
  if (recorder)
    recorder->EnableDebugRecording(std::move(file_provider));

  return recorder;
}
}  // namespace media
