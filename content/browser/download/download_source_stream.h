// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_SOURCE_STREAM_H_
#define CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_SOURCE_STREAM_H_

#include "content/browser/byte_stream.h"
#include "content/browser/download/download_interrupt_reasons_impl.h"
#include "content/public/browser/download_manager.h"
#include "content/public/common/download_stream.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/system/simple_watcher.h"

namespace content {
class ByteStreamReader;
// Wrapper of a ByteStreamReader or ScopedDataPipeConsumerHandle, and the meta
// data needed to write to a slice of the target file.
//
// Does not require the stream reader or the consumer handle to be ready when
// constructor is called. They can be added later when the network response
// is handled.
//
// Multiple SourceStreams can concurrently write to the same file sink.
class CONTENT_EXPORT DownloadSourceStream : public mojom::DownloadStreamClient {
 public:
  DownloadSourceStream(int64_t offset,
                       int64_t length,
                       std::unique_ptr<DownloadManager::InputStream> stream);
  ~DownloadSourceStream() override;

  void Initialize();

  // mojom::DownloadStreamClient
  void OnStreamCompleted(mojom::NetworkRequestStatus status) override;

  // Called when response is completed.
  void OnResponseCompleted(DownloadInterruptReason reason);

  // Called after successfully writing a buffer to disk.
  void OnWriteBytesToDisk(int64_t bytes_write);

  // Given a data block that is already written, truncate the length of this
  // object to avoid overwriting that block.
  void TruncateLengthWithWrittenDataBlock(int64_t offset,
                                          int64_t bytes_written);

  // Registers the callback that will be called when data is ready.
  void RegisterDataReadyCallback(
      const mojo::SimpleWatcher::ReadyCallback& callback);
  // Clears the callback that is registed when data is ready.
  void ClearDataReadyCallback();

  // Gets the status of the input stream when the stream completes.
  // TODO(qinmin): for data pipe, it currently doesn't support sending an
  // abort status at the end. The best way to do this is to add a separate
  // mojo interface for control messages when creating this object. See
  // http://crbug.com/748240. An alternative strategy is to let the
  // DownloadManager pass the status code to DownloadItem or DownloadFile.
  // However, a DownloadFile can have multiple SourceStreams, so we have to
  // maintain a map between data pipe and DownloadItem/DownloadFile somewhere.
  DownloadInterruptReason GetCompletionStatus();

  using CompletionCallback = base::OnceCallback<void(DownloadSourceStream*)>;
  // Register an callback to be called when download completes.
  void RegisterCompletionCallback(CompletionCallback callback);

  // Results for reading the DownloadSourceStream.
  enum StreamState {
    EMPTY = 0,
    HAS_DATA,
    WAIT_FOR_COMPLETION,
    COMPLETE,
  };
  StreamState Read(scoped_refptr<net::IOBuffer>* data, size_t* length);

  int64_t offset() const { return offset_; }
  int64_t length() const { return length_; }
  int64_t bytes_written() const { return bytes_written_; }
  bool is_finished() const { return finished_; }
  void set_finished(bool finish) { finished_ = finish; }
  size_t index() { return index_; }
  void set_index(size_t index) { index_ = index; }

 private:
  // Starting position for the stream to write to disk.
  int64_t offset_;

  // The maximum length to write to the disk. If set to 0, keep writing until
  // the stream depletes.
  int64_t length_;

  // Number of bytes written to disk from the stream.
  // Next write position is (|offset_| + |bytes_written_|).
  int64_t bytes_written_;

  // If all the data read from the stream has been successfully written to
  // disk.
  bool finished_;

  // The slice index in the |received_slices_| vector. A slice was created
  // once the stream started writing data to the target file.
  size_t index_;

  // The stream through which data comes.
  std::unique_ptr<ByteStreamReader> stream_reader_;

  // Status when the response completes, used by data pipe.
  DownloadInterruptReason completion_status_;

  // Whether the producer has completed handling the response.
  bool is_response_completed_;

  CompletionCallback completion_callback_;

  // Objects for consuming a mojo data pipe.
  mojom::DownloadStreamHandlePtr stream_handle_;
  std::unique_ptr<mojo::SimpleWatcher> handle_watcher_;
  std::unique_ptr<mojo::Binding<mojom::DownloadStreamClient>> binding_;

  DISALLOW_COPY_AND_ASSIGN(DownloadSourceStream);
};

}  // namespace content
#endif  // CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_SOURCE_STREAM_H_