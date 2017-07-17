// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/video/jpeg_decode_accelerator.h"

#include "base/bind.h"

namespace media {

JpegDecodeAccelerator::ClientOnTaskRunner::ClientOnTaskRunner(
    std::unique_ptr<Client> client,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : client_(std::move(client)),
      task_runner_(std::move(task_runner)),
      weak_factory_(this) {}

JpegDecodeAccelerator::ClientOnTaskRunner::~ClientOnTaskRunner() {
  task_runner_->DeleteSoon(FROM_HERE, client_.release());
}

void JpegDecodeAccelerator::ClientOnTaskRunner::VideoFrameReady(
    int32_t bitstream_buffer_id) {
  if (!task_runner_->BelongsToCurrentThread()) {
    task_runner_->PostTask(
        FROM_HERE, base::Bind(&Client::VideoFrameReady,
                              weak_factory_.GetWeakPtr(), bitstream_buffer_id));
    return;
  }
  client_->VideoFrameReady(bitstream_buffer_id);
}

void JpegDecodeAccelerator::ClientOnTaskRunner::NotifyError(
    int32_t bitstream_buffer_id,
    Error error) {
  if (!task_runner_->BelongsToCurrentThread()) {
    task_runner_->PostTask(
        FROM_HERE, base::Bind(&Client::NotifyError, weak_factory_.GetWeakPtr(),
                              bitstream_buffer_id, error));
    return;
  }
  client_->NotifyError(bitstream_buffer_id, error);
}

}  // namespace media
