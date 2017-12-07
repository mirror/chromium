// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_NET_CHUNKED_DATA_STREAM_UPLOADER_H_
#define IOS_NET_CHUNKED_DATA_STREAM_UPLOADER_H_

#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "net/base/upload_data_stream.h"

namespace net {
class IOBuffer;

// The ChunkedDataStreamUploader is used to support chunked data post for iOS
// NSMutableURLRequest HTTPBodyStream. Called on the network thread. It's
// responsible to coordinate the internal callbacks from network layer with the
// NSInputstream data. Rewind is not supported.
class ChunkedDataStreamUploader : public net::UploadDataStream {
 public:
  class Delegate {
   public:
    Delegate(){};
    virtual ~Delegate(){};

    // Called when the request is ready to read the data for request body data.
    // Data must be read in this function and put into |buf|. |buf_len| gives
    // the length of the provided buffer.
    virtual int OnRead(char* buf, int buf_len) = 0;

    // Called after all chunked data is uploaded. Notify the upper level to
    // handle the input stream.
    virtual void OnFinishUpload() = 0;
  };

  ChunkedDataStreamUploader(Delegate* delegate);
  ~ChunkedDataStreamUploader() override;

  // Interface for iOS layer to try to upload data. Used to notify the network
  // layer that iOS layer data is ready. Once it is ready for the request to
  // send for the network layer, the OnRead() callback will be called.
  void UploadWhenReady(bool final_chunk);

  // The uploader interface for iOS layer to use. It is weak point since the
  // owner of the uploader is the URLRequest.
  base::WeakPtr<ChunkedDataStreamUploader> GetUploader() {
    return weak_factory_.GetWeakPtr();
  }

 private:
  // Internal function to implement data upload to network layer.
  int Upload();

  // net::UploadDataStream implementation:
  int InitInternal(const NetLogWithSource& net_log) override;
  int ReadInternal(IOBuffer* buf, int buf_len) override;
  void ResetInternal() override;

  Delegate* const delegate_;

  // The pointer of the network layer buffer to send and the length of the
  // buffer.
  net::IOBuffer* pending_read_buf_;
  int pending_read_buf_len_;

  // Flags indicating current upload process has iOS stream pending or network
  // read callback pending.
  bool pending_stream_data_;
  bool pending_internal_read_;

  // Flags indicating if current block is the last.
  bool final_chunk_;

  // Set to false when a read starts, does not support rewinding
  // for stream upload.
  bool at_front_of_stream_;

  base::WeakPtrFactory<ChunkedDataStreamUploader> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ChunkedDataStreamUploader);
};

}  // namespace net

#endif  // IOS_NET_CHUNKED_DATA_STREAM_UPLOADER_H_
