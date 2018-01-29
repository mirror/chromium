// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "services/audio/public/cpp/debug_recording_session.h"
#include "services/audio/public/cpp/debug_recording_file_provider.h"
#include "services/audio/public/interfaces/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace audio {

DebugRecordingSession::DebugRecordingSession(
    service_manager::Connector* connector,
    const base::FilePath& file_name_base) {
  mojom::DebugRecordingFileProviderPtr file_provider;
  file_provider_ = std::make_unique<DebugRecordingFileProvider>(
      mojo::MakeRequest(&file_provider), file_name_base);
  debug_recording_.set_connection_error_handler(base::BindOnce(
      &DebugRecordingSession::OnConnectionError, base::Unretained(this)));
  connector->BindInterface(audio::mojom::kServiceName,
                           mojo::MakeRequest(&debug_recording_));
  // If BindInterface is unsuccessful, Enable will fail.
  // TODO remove comment after adding unit tests
  debug_recording_->Enable(std::move(file_provider));
}

DebugRecordingSession::~DebugRecordingSession() {
  debug_recording_.reset();
}

void DebugRecordingSession::OnConnectionError() {
  debug_recording_.reset();
}

}  // namespace audio
