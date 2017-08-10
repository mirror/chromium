// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STORAGE_BROWSER_BLOB_MOJO_BLOB_READER_H_
#define STORAGE_BROWSER_BLOB_MOJO_BLOB_READER_H_

#include "base/memory/ref_counted.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"
#include "net/base/net_errors.h"
#include "net/http/http_byte_range.h"
#include "storage/browser/blob/blob_reader.h"
#include "storage/browser/storage_browser_export.h"

namespace net {
class IOBufferWithSize;
}

namespace network {
class NetToMojoPendingBuffer;
}

namespace storage {
class BlobDataHandle;
class FileSystemContext;

// Reads a blob into a data pipe. Owns itself, and owns its delegate. Self
// destructs when reading is complete.
class STORAGE_EXPORT MojoBlobReader {
 public:
  class Delegate {
   public:
    enum RequestSideData { REQUEST_SIDE_DATA, DONT_REQUEST_SIDE_DATA };

    virtual ~Delegate() {}
    virtual mojo::ScopedDataPipeProducerHandle GetDataPipe() = 0;
    virtual RequestSideData DidCalculateSize(uint64_t total_size,
                                             uint64_t content_size) = 0;
    virtual void DidReadSideData(net::IOBufferWithSize* data) {}
    virtual void DidRead(int num_bytes) {}
    virtual void OnComplete(net::Error result,
                            uint64_t total_written_bytes) = 0;
  };

  static void Create(FileSystemContext* file_system_context,
                     const BlobDataHandle* handle,
                     const net::HttpByteRange& range,
                     std::unique_ptr<Delegate> delegate);

 private:
  MojoBlobReader(FileSystemContext* file_system_context,
                 const BlobDataHandle* handle,
                 const net::HttpByteRange& range,
                 std::unique_ptr<Delegate> delegate);
  ~MojoBlobReader();

  void Start();

  void NotifyCompletedAndDeleteIfNeeded(int result);

  void DidCalculateSize(int result);
  void DidReadSideData(BlobReader::Status status);
  void StartReading();
  void ReadMore();
  void DidRead(bool completed_synchronously, int num_bytes);
  void OnResponseBodyStreamClosed(MojoResult result);
  void OnResponseBodyStreamReady(MojoResult result);

  std::unique_ptr<Delegate> delegate_;

  net::HttpByteRange byte_range_;
  std::unique_ptr<BlobReader> blob_reader_;
  mojo::ScopedDataPipeProducerHandle response_body_stream_;
  scoped_refptr<network::NetToMojoPendingBuffer> pending_write_;
  mojo::SimpleWatcher writable_handle_watcher_;
  mojo::SimpleWatcher peer_closed_handle_watcher_;
  int64_t total_written_bytes_ = 0;
  bool notified_completed_ = false;

  base::WeakPtrFactory<MojoBlobReader> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MojoBlobReader);
};

}  // namespace storage

#endif
