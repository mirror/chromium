// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/services/media_util/public/cpp/safe_audio_video_checker.h"

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/time/time.h"
#include "chrome/services/media_util/public/interfaces/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

SafeAudioVideoChecker::SafeAudioVideoChecker(
    base::File file,
    const storage::CopyOrMoveFileValidator::ResultCallback& callback,
    std::unique_ptr<service_manager::Connector> connector)
    : file_(std::move(file)),
      connector_(std::move(connector)),
      callback_(callback) {
  DCHECK(callback_);
}

void SafeAudioVideoChecker::Start() {
  if (!file_.IsValid()) {
    callback_.Run(base::File::FILE_ERROR_SECURITY);
    return;
  }

  DCHECK(!media_parser_ptr_);

  connector_->BindInterface(chrome::mojom::kMediaUtilServiceName,
                            mojo::MakeRequest(&media_parser_ptr_));
  media_parser_ptr_.set_connection_error_handler(base::Bind(
      &SafeAudioVideoChecker::CheckMediaFileDone, this, /*valid=*/false));

  static constexpr auto kFileDecodeTime =
      base::TimeDelta::FromMilliseconds(250);

  media_parser_ptr_->CheckMediaFile(
      kFileDecodeTime, std::move(file_),
      base::Bind(&SafeAudioVideoChecker::CheckMediaFileDone, this));
}

void SafeAudioVideoChecker::CheckMediaFileDone(bool valid) {
  media_parser_ptr_.reset();  // Terminate the utility process.
  callback_.Run(valid ? base::File::FILE_OK : base::File::FILE_ERROR_SECURITY);
}

SafeAudioVideoChecker::~SafeAudioVideoChecker() = default;
