// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef SERVICES_AUDIO_DEBUG_RECORDING_H_
#define SERVICES_AUDIO_DEBUG_RECORDING_H_

#include "mojo/public/cpp/bindings/binding.h"
#include "services/audio/public/interfaces/debug_recording.mojom.h"

namespace media {
class AudioManager;
}

namespace audio {

class DebugRecording : public mojom::DebugRecording {
 public:
  DebugRecording(mojom::DebugRecordingRequest, media::AudioManager*);
  ~DebugRecording() override;

  void Enable(mojom::DebugRecordingFileProviderPtr file_provider) override;

 private:
  mojo::Binding<mojom::DebugRecording> binding_;
  mojom::DebugRecordingFileProviderPtr file_provider_;
  media::AudioManager* const audio_manager_;

  void CreateFile(
      const base::FilePath& file_suffix,
      mojom::DebugRecordingFileProvider::CreateCallback reply_callback);

  DISALLOW_COPY_AND_ASSIGN(DebugRecording);
};
}  // namespace audio

#endif  // SERVICES_AUDIO_DEBUG_RECORDING_H_
