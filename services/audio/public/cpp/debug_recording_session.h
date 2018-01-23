// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef SERVICES_AUDIO_DEBUG_RECORDING_SESSION_H_
#define SERVICES_AUDIO_DEBUG_RECORDING_SESSION_H_

#include "services/audio/public/interfaces/debug_recording.mojom.h"

namespace service_manager {
class Connector;
}

namespace audio {

class DebugRecordingFileProvider;

class DebugRecordingSession {
 public:
  DebugRecordingSession(service_manager::Connector* connector,
                        const base::FilePath& file_name_base);
  ~DebugRecordingSession();

 private:
  void OnConnectionError();

  std::unique_ptr<DebugRecordingFileProvider> file_provider_;
  mojom::DebugRecordingPtr debug_recording_;

  DISALLOW_COPY_AND_ASSIGN(DebugRecordingSession);
};

}  // namespace audio
#endif  // SERVICES_AUDIO_DEBUG_RECORDING_SESSION_H_
