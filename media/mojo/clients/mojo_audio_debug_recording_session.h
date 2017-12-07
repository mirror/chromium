// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef MEDIA_MOJO_CLIENTS_MOJO_AUDIO_DEBUG_RECORDING_SESSION_H_
#define MEDIA_MOJO_CLIENTS_MOJO_AUDIO_DEBUG_RECORDING_SESSION_H_

#include "media/mojo/interfaces/audio_debug_recording.mojom.h"
#include "media/mojo/interfaces/audio_debug_recording_file_provider.mojom.h"
#include "media/mojo/services/media_mojo_export.h"

namespace media {

class MEDIA_MOJO_EXPORT AudioDebugRecordingSession {
 public:
  mojom::AudioDebugRecordingPtr Create(
      mojom::AudioDebugRecordingFileProviderPtr file_provider);
};

}  // namespace media
#endif  // MEDIA_MOJO_CLIENTS_MOJO_AUDIO_DEBUG_RECORDING_SESSION_H_
