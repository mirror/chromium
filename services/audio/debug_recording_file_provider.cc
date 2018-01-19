// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "services/audio/debug_recording_file_provider.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace audio {

DebugRecordingFileProvider::DebugRecordingFileProvider(
    mojom::DebugRecordingFileProviderRequest request,
    const base::FilePath& file_name_base)
    : binding_(this, std::move(request)), file_name_base_(file_name_base) {}

DebugRecordingFileProvider::~DebugRecordingFileProvider() {}

void DebugRecordingFileProvider::Create(const base::FilePath& file_suffix,
                                        CreateCallback reply_callback) {
  base::File debug_recording_file(
      file_name_base_.Append(file_suffix),
      base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
  std::move(reply_callback).Run(std::move(debug_recording_file));
}

}  // namespace audio
