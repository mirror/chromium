// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MOJO_DATA_SOURCE_ADAPTER_H_
#define MEDIA_MOJO_SERVICES_MOJO_DATA_SOURCE_ADAPTER_H_

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "media/base/data_source.h"
#include "media/mojo/interfaces/data_source.mojom.h"

namespace media {

class DataBuffer;
class MojoDataBufferReader;

// This class acts as a MojoRendererService-side stub for a real DemuxerStream
// that is part of a Pipeline in a remote application. Roughly speaking, it
// takes a mojom::DemuxerStreamPtr and exposes it as a DemuxerStream for
// use by
// media components.
class MojoDataSourceAdapter : public DataSource {
 public:
  // |data_buffer| is connected to the mojom::DataSource that |this|
  // will become the client of.
  MojoDataSourceAdapter(mojom::DataSourcePtr data_source,
                        const base::Closure& data_source_ready_cb);
  ~MojoDataSourceAdapter() override;

  // DataSource implementation.
  void Read(int64_t position,
            int size,
            uint8_t* data,
            const DataSource::ReadCB& read_cb) override;
  void Stop() override;
  void Abort() override;
  bool GetSize(int64_t* size_out) override;
  bool IsStreaming() override;
  void SetBitrate(int bitrate) override;

 private:
  void OnDataSourceReady(const base::Closure& data_source_ready_cb,
                         mojo::ScopedDataPipeConsumerHandle consumer_handle);
  void OnDataSourceRead(uint8_t* data,
                        const DataSource::ReadCB& read_cb,
                        mojom::DataBufferPtr buffer,
                        int size);
  void OnBufferRead(uint8_t* data,
                    const DataSource::ReadCB& read_cb,
                    int size,
                    scoped_refptr<media::DataBuffer> buffer);
  void GetSizeInternal(bool* success, int64_t* size_out);

  std::mutex mutex_;
  std::condition_variable condition_;

  // See constructor for descriptions.
  mojom::DataSourcePtr data_source_;

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  std::unique_ptr<MojoDataBufferReader> mojo_data_buffer_reader_;

  base::WeakPtrFactory<MojoDataSourceAdapter> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(MojoDataSourceAdapter);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MOJO_DATA_SOURCE_ADAPTER_H_
