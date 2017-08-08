// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_FILE_TRANSFER_PROXY_H_
#define REMOTING_HOST_FILE_TRANSFER_PROXY_H_

#include "base/callback.h"
#include "base/files/file_proxy.h"
#include "remoting/proto/file_transfer.pb.h"

namespace remoting {

class CompoundBuffer;

// FileTransferProxy is an interface for implementing platform-specific file
// writers for file transfers. Each operation is posted to a separate file IO
// thread, and possibly a different process depending on the platform.
class FileTransferProxy {
 public:
  FileTransferProxy();
  virtual ~FileTransferProxy();

  virtual void Open(const std::string& filename,
                    uint64_t filesize,
                    const base::FileProxy::StatusCallback& callback) = 0;
  virtual void WriteChunk(std::unique_ptr<CompoundBuffer> buffer,
                          const base::FileProxy::WriteCallback& callback) = 0;
  virtual void Close(const base::FileProxy::StatusCallback& callback) = 0;
  virtual void Cancel(const base::FileProxy::StatusCallback& callback) = 0;
};

}  // namespace remoting

#endif  // REMOTING_HOST_FILE_TRANSFER_PROXY_H_
