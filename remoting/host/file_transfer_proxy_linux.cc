// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/file_transfer_proxy_linux.h"

#include "remoting/base/compound_buffer.h"

namespace remoting {

FileTransferProxyLinux::FileTransferProxyLinux(
    scoped_refptr<base::SingleThreadTaskRunner> file_task_runner) {}

FileTransferProxyLinux::~FileTransferProxyLinux() = default;

void FileTransferProxyLinux::Open(
    const std::string& filename,
    uint64_t filesize,
    const base::FileProxy::StatusCallback& callback) {
  // TODO(jarhar): Implement Open.
}

void FileTransferProxyLinux::WriteChunk(
    std::unique_ptr<CompoundBuffer> buffer,
    const base::FileProxy::WriteCallback& callback) {
  // TODO(jarhar): Implement WriteChunk.
}

void FileTransferProxyLinux::Close(
    const base::FileProxy::StatusCallback& callback) {
  // TODO(jarhar): Implement Close.
}

void FileTransferProxyLinux::Cancel(
    const base::FileProxy::StatusCallback& callback) {
  // TODO(jarhar): Implement Cancel.
}

}  // namespace remoting
