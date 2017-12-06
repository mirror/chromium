// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef MEDIA_MOJO_SERVICES_MOJO_AUDIO_DEBUG_RECORDING_FILE_PROVIDER_H_
#define MEDIA_MOJO_SERVICES_MOJO_AUDIO_DEBUG_RECORDING_FILE_PROVIDER_H_

#include "media/mojo/interfaces/audio_debug_recording.mojom.h"
#include "media/mojo/services/media_mojo_export.h"
#include "mojo/common/file.mojom.h"
#include "mojo/common/file_path.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace media {

class MEDIA_MOJO_EXPORT MojoAudioDebugRecordingFileProvider
    : public mojom::AudioDebugRecordingFileProvider {
 public:
  MojoAudioDebugRecordingFileProvider(
      mojom::AudioDebugRecordingFileProviderRequest request,
      const base::FilePath& file_name_base);
  ~MojoAudioDebugRecordingFileProvider() override;

  void GetFileHandle(const base::FilePath& file_suffix,
                     GetFileHandleCallback cb) override;
  void IsValid(base::File file_handle, IsValidCallback cb) override;

 private:
  mojo::Binding<mojom::AudioDebugRecordingFileProvider> binding_;
  base::FilePath file_name_base_;

  DISALLOW_COPY_AND_ASSIGN(MojoAudioDebugRecordingFileProvider);
};
}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MOJO_AUDIO_DEBUG_RECORDING_FILE_PROVIDER_H_
