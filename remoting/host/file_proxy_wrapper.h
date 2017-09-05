// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_FILE_PROXY_WRAPPER_H_
#define REMOTING_HOST_FILE_PROXY_WRAPPER_H_

#include "base/callback.h"
#include "base/files/file_proxy.h"
#include "base/optional.h"
#include "remoting/proto/file_transfer.pb.h"

namespace remoting {

class CompoundBuffer;

// FileProxyWrapper is an interface for implementing platform-specific file
// writers for file transfers. Each operation is posted to a separate file IO
// thread, and possibly a different process depending on the platform.
class FileProxyWrapper {
 public:
  enum State {
    // Created, but Init() has not been called yet.
    kUninitialized = 0,

    // Init() has been called.
    kInitialized = 1,

    // CreateFile() has been called. The file may or may not exist yet, but
    // this means that WriteChunk() can now be called. This state is only used
    // when writing to a file.
    kFileCreated = 2,

    // Close() has been called. WriteChunk() can no longer be called, but not
    // all chunks may have been written to disk yet. After chunks are written,
    // the file will be moved to its target location. This state is only used
    // when writing to a file.
    kClosing = 3,

    // OpenFile() has been called. ReadChunk() can now be called. This state is
    // only used when reading from a file.
    kFileOpened = 4,

    // ReadChunk() has been called and FileProxyWrapper is waiting on a read
    // operation. ReadChunk() cannot be called again until the read callback is
    // run and the state changes back to kFileOpened. This state is only used
    // when reading from a file.
    kReading = 5,

    // Close() has been called and succeeded.
    kClosed = 6,

    // Cancel() has been called or an error occured.
    kFailed = 7,
  };

  // If an error occured while writing the file, State will be kFailed and the
  // Optional will contain the error which occured. If the file was written to
  // and closed successfully, State will be kClosed and the Optional will be
  // empty.
  typedef base::OnceCallback<
      void(State, base::Optional<protocol::FileTransferResponse_ErrorCode>)>
      StatusCallback;

  // Callback used when opening a file for reading. The int64_t parameter is the
  // size of the file.
  typedef base::OnceCallback<void(int64_t)> OpenFileCallback;

  typedef base::OnceCallback<void(std::unique_ptr<std::vector<char>>)>
      ReadCallback;

  // Creates a platform-specific FileProxyWrapper.
  static std::unique_ptr<FileProxyWrapper> Create();

  FileProxyWrapper();
  virtual ~FileProxyWrapper();

  // |status_callback| is called either when FileProxyWrapper encounters an
  // error or when Close() has been called and the file has been written
  // successfully. |status_callback| must not immediately destroy this
  // FileProxyWrapper.
  virtual void Init(StatusCallback status_callback) = 0;
  // Creates a new file and opens it for writing.
  virtual void CreateFile(const base::FilePath& directory,
                          const std::string& filename) = 0;
  // Opens an existing file for reading.
  virtual void OpenFile(const base::FilePath& filepath,
                        OpenFileCallback open_callback) = 0;
  virtual void WriteChunk(std::unique_ptr<CompoundBuffer> buffer) = 0;
  // |size| must not be greater than the remaining amount of bytes in the file
  // from the current read offset. After calling ReadChunk(), ReadChunk() cannot
  // be called again until |read_callback| is called and state() returns
  // kFileOpened.
  virtual void ReadChunk(uint64_t size, ReadCallback read_callback) = 0;
  virtual void Close() = 0;
  virtual void Cancel() = 0;
  virtual State state() = 0;
};

}  // namespace remoting

#endif  // REMOTING_HOST_FILE_PROXY_WRAPPER_H_
