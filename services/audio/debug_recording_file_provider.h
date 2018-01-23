// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef SERVICES_AUDIO_DEBUG_RECORDING_FILE_PROVIDER_H_
#define SERVICES_AUDIO_DEBUG_RECORDING_FILE_PROVIDER_H_

#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/audio/public/interfaces/debug_recording.mojom.h"

namespace base {
class FilePath;
}  // namespace base

namespace audio {

class DebugRecordingFileProvider : public mojom::DebugRecordingFileProvider {
 public:
  explicit DebugRecordingFileProvider(const base::FilePath& file_name_base);
  ~DebugRecordingFileProvider() override;

  void BindRequest(mojom::DebugRecordingFileProviderRequest request);
  void Create(const base::FilePath& file_suffix,
              CreateCallback reply_callback) override;

 private:
  mojo::BindingSet<mojom::DebugRecordingFileProvider> bindings_;
  base::FilePath file_name_base_;

  DISALLOW_COPY_AND_ASSIGN(DebugRecordingFileProvider);
};
}  // namespace audio

#endif  // SERVICES_AUDIO_DEBUG_RECORDING_FILE_PROVIDER_H_
