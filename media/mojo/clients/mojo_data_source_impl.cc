// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/clients/mojo_data_source_impl.h"

#include "media/base/data_buffer.h"
#include "media/mojo/common/media_type_converters.h"
#include "media/mojo/common/mojo_data_buffer_converter.h"

namespace media {

MojoDataSourceImpl::MojoDataSourceImpl(
    media::DataSource* data_source,
    mojo::InterfaceRequest<mojom::DataSource> request)
    : binding_(this, std::move(request)),
      data_source_(data_source),
      weak_factory_(this) {
  CHECK(data_source_);
}

MojoDataSourceImpl::~MojoDataSourceImpl() {}

// This is called when our DataSourceClient has connected itself and is
// ready to receive messages.  Send an initial config and notify it that
// we are now ready for business.
void MojoDataSourceImpl::Initialize(InitializeCallback callback) {
  DVLOG(2) << __func__;

  mojo::ScopedDataPipeConsumerHandle remote_consumer_handle;

  // TODO(j.isorce): fix TODO in media/mojo/common/mojo_data_buffer_converter.cc
  // CreateDataPipe first and reflect the fix here. For now the following value
  // should be enough for audio + video.
  // Other option is to set the capacity as a multiple of the bitrate got from
  // SetBitrate below.
  mojo_data_buffer_writer_ = MojoDataBufferWriter::Create(
      /* capacity_num_bytes */ 3 * 1024 * 1024, &remote_consumer_handle);

  std::move(callback).Run(std::move(remote_consumer_handle));
}

void MojoDataSourceImpl::Read(int64_t position,
                              int size,
                              ReadCallback callback) {
  std::unique_ptr<uint8_t[]> data(new uint8_t[size]);

  data_source_->Read(
      position, size, data.get(),
      base::Bind(&MojoDataSourceImpl::OnRead, weak_factory_.GetWeakPtr(),
                 base::Passed(&callback), base::Passed(std::move(data))));
}

void MojoDataSourceImpl::OnRead(ReadCallback callback,
                                std::unique_ptr<uint8_t[]> data,
                                int size) {
  mojom::DataBufferPtr mojo_buffer;
  if (size != media::DataSource::kReadError &&
      size != media::DataSource::kAborted) {
    scoped_refptr<media::DataBuffer> buffer =
        new media::DataBuffer(std::move(data), size);
    mojo_buffer = mojo_data_buffer_writer_->WriteDataBuffer(buffer);
  }
  std::move(callback).Run(std::move(mojo_buffer), size);
}

void MojoDataSourceImpl::Stop() {
  data_source_->Stop();
}

void MojoDataSourceImpl::Abort() {
  data_source_->Abort();
}

void MojoDataSourceImpl::GetSize(GetSizeCallback callback) {
  int64_t size = 0;
  bool success = data_source_->GetSize(&size);
  std::move(callback).Run(success, size);
}

void MojoDataSourceImpl::IsStreaming(IsStreamingCallback callback) {
  bool is_streaming = data_source_->IsStreaming();
  std::move(callback).Run(is_streaming);
}

void MojoDataSourceImpl::SetBitrate(int bitrate) {
  data_source_->SetBitrate(bitrate);
}

}  // namespace media
