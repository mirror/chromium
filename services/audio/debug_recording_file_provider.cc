// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <utility>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "services/audio/debug_recording_file_provider.h"

namespace audio {

DebugRecordingFileProvider::DebugRecordingFileProvider(
    const base::FilePath& file_name_base)
    : file_name_base_(file_name_base) {}

DebugRecordingFileProvider::~DebugRecordingFileProvider() {}

void DebugRecordingFileProvider::BindRequest(
    mojom::DebugRecordingFileProviderRequest request) {
  bindings_.AddBinding(this, std::move(request));
}
void DebugRecordingFileProvider::Create(const base::FilePath& file_suffix,
                                        CreateCallback reply_callback) {
  base::File debug_recording_file(
      file_name_base_.InsertBeforeExtension(file_suffix.value()),
      base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
  std::move(reply_callback).Run(std::move(debug_recording_file));
}

}  // namespace audio
