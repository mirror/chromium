// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/file_transfer_proxy_factory_linux.h"

#include "base/memory/ptr_util.h"
#include "remoting/host/file_transfer_proxy_linux.h"

namespace remoting {

FileTransferProxyFactoryLinux::FileTransferProxyFactoryLinux(
    scoped_refptr<base::SingleThreadTaskRunner> file_task_runner)
    : file_task_runner_(file_task_runner) {}

FileTransferProxyFactoryLinux::~FileTransferProxyFactoryLinux() = default;

std::unique_ptr<FileTransferProxy>
FileTransferProxyFactoryLinux::CreateProxy() {
  return base::WrapUnique(new FileTransferProxyLinux(file_task_runner_));
}

}  // namespace remoting
