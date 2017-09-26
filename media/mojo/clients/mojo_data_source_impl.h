// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_CLIENTS_MOJO_DATA_SOURCE_IMPL_H_
#define MEDIA_MOJO_CLIENTS_MOJO_DATA_SOURCE_IMPL_H_

#include <memory>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "media/base/data_source.h"
#include "media/mojo/interfaces/data_source.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace media {

class DataSource;
class MojoDataBufferWriter;

// This class wraps a media::DataSource and exposes it as a
// mojom::DataSource for use as a proxy from remote applications.
class MojoDataSourceImpl : public mojom::DataSource {
 public:
  // |data_source| is the underlying DataSource we are proxying for.
  // Note: |this| does not take ownership of |data_source|.
  MojoDataSourceImpl(media::DataSource* data_source,
                     mojo::InterfaceRequest<mojom::DataSource> request);
  ~MojoDataSourceImpl() override;

  // mojom::DataSource implementation.
  void Initialize(InitializeCallback callback) final;
  void Read(int64_t position, int size, ReadCallback callback) final;
  void Stop() final;
  void Abort() final;
  void GetSize(GetSizeCallback callback) final;
  void IsStreaming(IsStreamingCallback callback) final;
  void SetBitrate(int bitrate) final;

  // Sets an error handler that will be called if a connection error occurs on
  // the bound message pipe.
  void set_connection_error_handler(const base::Closure& error_handler) {
    binding_.set_connection_error_handler(error_handler);
  }

 private:
  void OnRead(const ReadCallback callback,
              std::unique_ptr<uint8_t[]> data,
              int size);

  mojo::Binding<mojom::DataSource> binding_;

  // See constructor.  We do not own |stream_|.
  media::DataSource* data_source_;

  std::unique_ptr<MojoDataBufferWriter> mojo_data_buffer_writer_;

  base::WeakPtrFactory<MojoDataSourceImpl> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(MojoDataSourceImpl);
};

}  // namespace media

#endif  // MEDIA_MOJO_CLIENTS_MOJO_DATA_SOURCE_IMPL_H_
