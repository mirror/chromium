// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_FILE_TRANSFER_HANDLER_H_
#define REMOTING_HOST_FILE_TRANSFER_HANDLER_H_

#include "base/callback.h"
#include "base/files/file_proxy.h"
#include "remoting/proto/file_transfer.pb.h"

namespace remoting {

class CompoundBuffer;

class FileTransferHandler {
 public:
  typedef base::OnceCallback<void(protocol::FileTransferResponse_ErrorCode)>
      HandlerCallback;

  FileTransferHandler();
  virtual ~FileTransferHandler();

  virtual void Open(HandlerCallback callback) = 0;
  virtual void WriteChunk(HandlerCallback callback,
                          std::unique_ptr<CompoundBuffer> buffer) = 0;
  virtual void Close(HandlerCallback callback) = 0;
  virtual void Cancel(HandlerCallback callback) = 0;
};

}  // namespace remoting

#endif  // REMOTING_HOST_FILE_TRANSFER_HANDLER_H_
