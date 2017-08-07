// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FILE_TRANSFER_PROXY_FACTORY_LINUX_H_
#define FILE_TRANSFER_PROXY_FACTORY_LINUX_H_

#include "base/single_thread_task_runner.h"
#include "remoting/host/file_transfer_proxy_factory.h"

namespace remoting {

class FileTransferProxyFactoryLinux : public FileTransferProxyFactory {
 public:
  FileTransferProxyFactoryLinux(
      scoped_refptr<base::SingleThreadTaskRunner> file_task_runner);
  ~FileTransferProxyFactoryLinux() override;

  // FileTransferProxyFactory implementation.
  std::unique_ptr<FileTransferProxy> CreateProxy() override;

 private:
  scoped_refptr<base::SingleThreadTaskRunner> file_task_runner_;
};

}  // namespace remoting

#endif  // FILE_TRANSFER_PROXY_FACTORY_LINUX_H_
