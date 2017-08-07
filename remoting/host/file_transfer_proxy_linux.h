// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_FILE_TRANSFER_PROXY_LINUX_H_
#define REMOTING_HOST_FILE_TRANSFER_PROXY_LINUX_H_

#include "base/single_thread_task_runner.h"
#include "remoting/host/file_transfer_proxy.h"

namespace remoting {

class FileTransferProxyLinux : public FileTransferProxy {
 public:
  FileTransferProxyLinux(
      scoped_refptr<base::SingleThreadTaskRunner> file_task_runner);
  ~FileTransferProxyLinux() override;

  // FileTransferProxy implementation.
  void Open(const std::string& filename,
            uint64_t filesize,
            const base::FileProxy::StatusCallback& callback) override;
  void WriteChunk(std::unique_ptr<CompoundBuffer> buffer,
                  const base::FileProxy::WriteCallback& callback) override;
  void Close(const base::FileProxy::StatusCallback& callback) override;
  void Cancel(const base::FileProxy::StatusCallback& callback) override;
};

}  // namespace remoting

#endif  // REMOTING_HOST_FILE_TRANSFER_PROXY_LINUX_H_
