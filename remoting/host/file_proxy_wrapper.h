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
  // If the unique_ptr is empty, the file was written to successfully. If
  // present, the ErrorCode represents the error FileProxyWrapper encountered.
  typedef base::OnceCallback<void(
      std::unique_ptr<protocol::FileTransferResponse_ErrorCode>)>
      DoneCallback;

  enum State {
    // Created, but Init() has not been called yet.
    kUninitialized = 0,

    // Init() has been called.
    kInitialized = 1,

    // CreateFile() has been called. The file may or may not exist yet, but
    // this means that WriteChunk() can now be called.
    kFileCreated = 2,

    // Close() has been called. WriteChunk() can no longer be called, but not
    // all chunks may have been written to disk yet. After chunks are written,
    // the file will be moved to its target location.
    kClosing = 3,

    // Close() has been called and succeeded.
    kClosed = 4,

    // Cancel() has been called or an error occured.
    kFailed = 5,
  };

  // Creates a platform-specific FileProxyWrapper.
  static std::unique_ptr<FileProxyWrapper> Create();

  FileProxyWrapper();
  virtual ~FileProxyWrapper();

  // |done_callback| is called either when FileProxyWrapper encounters an error
  // or when Close() has been called and the file has been written successfully.
  // |done_callback| must not immediately destroy this FileProxyWrapper.
  virtual void Init(DoneCallback done_callback) = 0;
  virtual void CreateFile(const base::FilePath& directory,
                          const std::string& filename) = 0;
  virtual void WriteChunk(std::unique_ptr<CompoundBuffer> buffer) = 0;
  virtual void Close() = 0;
  virtual void Cancel() = 0;
  virtual State state() = 0;
};

}  // namespace remoting

#endif  // REMOTING_HOST_FILE_PROXY_WRAPPER_H_
