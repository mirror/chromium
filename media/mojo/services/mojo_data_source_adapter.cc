// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mojo_data_source_adapter.h"

#include <stdint.h>
#include <utility>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/numerics/safe_conversions.h"
#include "media/base/data_buffer.h"
#include "media/mojo/common/media_type_converters.h"
#include "media/mojo/common/mojo_data_buffer_converter.h"
#include "mojo/public/cpp/system/data_pipe.h"

namespace media {

MojoDataSourceAdapter::MojoDataSourceAdapter(
    mojom::DataSourcePtr data_source,
    const base::Closure& data_source_ready_cb)
    : data_source_(std::move(data_source)),
      task_runner_(base::ThreadTaskRunnerHandle::Get()),
      weak_factory_(this) {
  DVLOG(1) << __func__;
  data_source_->Initialize(base::Bind(&MojoDataSourceAdapter::OnDataSourceReady,
                                      weak_factory_.GetWeakPtr(),
                                      data_source_ready_cb));
}

MojoDataSourceAdapter::~MojoDataSourceAdapter() {
  DVLOG(1) << __func__;
}

void MojoDataSourceAdapter::Read(int64_t position,
                                 int size,
                                 uint8_t* data,
                                 const DataSource::ReadCB& read_cb) {
  DVLOG(3) << __func__;

  data_source_->Read(position, size,
                     base::Bind(&MojoDataSourceAdapter::OnDataSourceRead,
                                weak_factory_.GetWeakPtr(), data, read_cb));
}

void MojoDataSourceAdapter::Stop() {
  data_source_->Stop();
}

void MojoDataSourceAdapter::Abort() {
  data_source_->Abort();
}

bool MojoDataSourceAdapter::GetSize(int64_t* size_out) {
  bool success = false;

  if (!task_runner_->BelongsToCurrentThread()) {
    std::unique_lock<std::mutex> lock(mutex_);
    task_runner_->PostTask(
        FROM_HERE, base::Bind(&MojoDataSourceAdapter::GetSizeInternal,
                              weak_factory_.GetWeakPtr(), &success, size_out));
    condition_.wait(lock);
  } else {
    MojoDataSourceAdapter::GetSizeInternal(&success, size_out);
  }

  return success;
}

bool MojoDataSourceAdapter::IsStreaming() {
  CHECK(task_runner_->BelongsToCurrentThread());
  bool is_streaming = false;
  data_source_->IsStreaming(&is_streaming);
  return is_streaming;
}

void MojoDataSourceAdapter::SetBitrate(int bitrate) {
  data_source_->SetBitrate(bitrate);
}

void MojoDataSourceAdapter::OnDataSourceReady(
    const base::Closure& data_source_ready_cb,
    mojo::ScopedDataPipeConsumerHandle consumer_handle) {
  DVLOG(1) << __func__;
  DCHECK(consumer_handle.is_valid());

  mojo_data_buffer_reader_.reset(
      new MojoDataBufferReader(std::move(consumer_handle)));

  data_source_ready_cb.Run();
}

void MojoDataSourceAdapter::OnDataSourceRead(uint8_t* data,
                                             const DataSource::ReadCB& read_cb,
                                             mojom::DataBufferPtr buffer,
                                             int size) {
  DVLOG(3) << __func__;

  if (size == media::DataSource::kAborted ||
      size == media::DataSource::kReadError) {
    read_cb.Run(size);
    return;
  }

  mojo_data_buffer_reader_->ReadDataBuffer(
      std::move(buffer),
      base::BindOnce(&MojoDataSourceAdapter::OnBufferRead,
                     weak_factory_.GetWeakPtr(), data, read_cb, size));
}

void MojoDataSourceAdapter::OnBufferRead(
    uint8_t* data,
    const DataSource::ReadCB& read_cb,
    int size,
    scoped_refptr<media::DataBuffer> buffer) {
  if (buffer) {
    CHECK(buffer->data_size() == size);
    memcpy(data, buffer->data(), buffer->data_size());
  }

  read_cb.Run(size);
}

void MojoDataSourceAdapter::GetSizeInternal(bool* success, int64_t* size_out) {
  // Sync calls must be done from the bound thread.
  CHECK(task_runner_->BelongsToCurrentThread());

  std::unique_lock<std::mutex> lock(mutex_);
  data_source_->GetSize(success, size_out);

  lock.unlock();
  condition_.notify_one();
}

}  // namespace media
