// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef MEDIA_MOJO_SERVICES_MOJO_AUDIO_DEBUG_RECORDING_H_
#define MEDIA_MOJO_SERVICES_MOJO_AUDIO_DEBUG_RECORDING_H_

#include "media/mojo/interfaces/audio_debug_recording.mojom.h"
#include "media/mojo/services/media_mojo_export.h"
#include "media/mojo/services/mojo_audio_debug_recording_file_provider.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace media {

class AudioDebugRecordingManager;

class MEDIA_MOJO_EXPORT MojoDebugRecording : public mojom::DebugRecording {
 public:
  MojoDebugRecording(mojom::DebugRecordingRequest, AudioDebugRecordingManager*);
  ~MojoDebugRecording() override;

  void Enable(DebugRecordingFileProviderPtr file_provider) override;

 private:
  mojo::Binding<mojom::DebugRecording> binding_;
  AudioDebugRecordingManager* const recording_manager_;

  DISALLOW_COPY_AND_ASSIGN(MojoDebugRecording);
};
}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MOJO_AUDIO_DEBUG_RECORDING_H_
