// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_FILEAPI_FILE_STREAM_FORWARDER_H_
#define CHROME_BROWSER_CHROMEOS_ARC_FILEAPI_FILE_STREAM_FORWARDER_H_

#include <memory>

#include "base/callback.h"
#include "base/files/scoped_file.h"
#include "base/memory/weak_ptr.h"
#include "net/base/io_buffer.h"
#include "storage/browser/fileapi/file_stream_reader.h"
#include "storage/browser/fileapi/file_system_context.h"

namespace arc {

// FileStreamForwarder reads data of the given FileSystemURL, and writes it to
// the given FD. While all internal actions happen on the IO thread, all public
// methods should be called on the UI thread. If this object is destroyed before
// the completion, the in-flight operation may abort before finishing, and the
// callback will be called with false when aborted.
class FileStreamForwarder {
 public:
  struct DestroyHelper {
    void operator()(FileStreamForwarder* object) const { object->Destroy(); }
  };

  // |result| is true when the specified amount of data was successfully
  // forwarded.
  using ResultCallback = base::OnceCallback<void(bool result)>;

  FileStreamForwarder(scoped_refptr<storage::FileSystemContext> context,
                      const storage::FileSystemURL& url,
                      int64_t offset,
                      int64_t size,
                      base::ScopedFD fd_dest,
                      ResultCallback callback);

  // Posts a task to destruct this object on the IO thread.
  void Destroy();

 private:
  // Destructs this object.
  void DestroyOnIOThread();

  // Use Destroy() to destruct this object.
  ~FileStreamForwarder();

  // Starts reading the data.
  void Start();

  // Do the actual reading.
  void DoRead();

  // Called when read is completed.
  void OnReadCompleted(int result);

  // Called when write is completed.
  void OnWriteCompleted(bool result);

  // Runs the result callback on the UI thread.
  void NotifyCompleted(bool result);

  scoped_refptr<storage::FileSystemContext> context_;
  const storage::FileSystemURL url_;
  const int64_t offset_;
  int64_t remaining_size_;
  base::ScopedFD fd_dest_;
  ResultCallback callback_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;  // For blocking IO.
  scoped_refptr<net::IOBufferWithSize> buf_;
  std::unique_ptr<storage::FileStreamReader> stream_reader_;

  base::WeakPtrFactory<FileStreamForwarder> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(FileStreamForwarder);
};

using FileStreamForwarderPtr =
    std::unique_ptr<FileStreamForwarder, FileStreamForwarder::DestroyHelper>;

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_FILEAPI_FILE_STREAM_FORWARDER_H_
