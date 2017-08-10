// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_FILE_TRANSFER_PROXY_FACTORY_H_
#define REMOTING_HOST_FILE_TRANSFER_PROXY_FACTORY_H_

#include "remoting/host/file_transfer_proxy.h"

namespace remoting {

// FileTransferProxyFactory is an interface for creating platform-specific
// FileTransferProxies.
class FileTransferProxyFactory {
 public:
  FileTransferProxyFactory();
  virtual ~FileTransferProxyFactory();

  virtual std::unique_ptr<FileTransferProxy> CreateProxy() = 0;
};

}  // namespace remoting

#endif  // REMOTING_HOST_FILE_TRANSFER_PROXY_FACTORY_H_
