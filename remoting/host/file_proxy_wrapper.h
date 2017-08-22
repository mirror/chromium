// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_FILE_PROXY_WRAPPER_H_
#define REMOTING_HOST_FILE_PROXY_WRAPPER_H_

#include "base/callback.h"
#include "base/files/file_proxy.h"
#include "remoting/proto/file_transfer.pb.h"

namespace remoting {

class CompoundBuffer;

// FileProxyWrapper is an interface for implementing platform-specific file
// writers for file transfers. Each operation is posted to a separate file IO
// thread, and possibly a different process depending on the platform.
class FileProxyWrapper {
 public:
  typedef base::Callback<void(protocol::FileTransferResponse_ErrorCode)>
      ErrorCallback;
  typedef base::Callback<void()> SuccessCallback;

  enum State {
    // Created, but Init() has not been called yet.
    UNINITIALIZED = 1,

    // Init() has been called.
    INITIALIZED = 2,

    // CreateFile() has been called and succeeded.
    FILE_CREATED = 3,

    // Close() has been called. WriteChunk() can no longer be called, but not
    // all chunks may have been written to disk yet. After chunks are written,
    // the file will be moved to its target location.
    CLOSING = 4,

    // Close() has been called and succeeded.
    COMPLETED = 5,

    // Cancel() has been called or an error occured.
    CANCELLED = 6,
  };

  // Creates a platform-specific FileProxyWrapper.
  static std::unique_ptr<FileProxyWrapper> Create();

  FileProxyWrapper();
  virtual ~FileProxyWrapper();

  virtual void Init(const ErrorCallback& error_callback) = 0;
  virtual void CreateFile(const base::FilePath& directory,
                          const std::string& filename,
                          const SuccessCallback& success_callback) = 0;
  virtual void WriteChunk(std::unique_ptr<CompoundBuffer> buffer) = 0;
  virtual void Close(const SuccessCallback& success_callback) = 0;
  virtual void Cancel() = 0;
  virtual State state() = 0;
};

}  // namespace remoting

#endif  // REMOTING_HOST_FILE_PROXY_WRAPPER_H_
