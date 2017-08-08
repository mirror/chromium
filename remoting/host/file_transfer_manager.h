// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_FILE_TRANSFER_MANAGER_H_
#define REMOTING_HOST_FILE_TRANSFER_MANAGER_H_

#include "remoting/host/file_transfer_handler.h"

namespace remoting {

class FileTransferManager {
 public:
  FileTransferManager();
  virtual ~FileTransferManager();

  virtual base::WeakPtr<FileTransferHandler> CreateHandler(
      const protocol::FileTransferRequest& request) = 0;
};

}  // namespace remoting

#endif  // REMOTING_HOST_FILE_TRANSFER_MANAGER_H_
