// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/file_proxy_wrapper.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "base/files/file_util.h"
#include "base/memory/ptr_util.h"
#include "base/sequenced_task_runner.h"
#include "base/task_runner_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_checker.h"
#include "remoting/base/compound_buffer.h"

namespace {

constexpr char kTempFileExtension[] = ".crdownload";

void EmptyStatusCallback(base::File::Error error) {}

void EmptyDeleteFileCallback(bool success) {}

remoting::protocol::FileTransferResponse_ErrorCode FileErrorToResponseError(
    base::File::Error file_error) {
  switch (file_error) {
    case base::File::FILE_ERROR_ACCESS_DENIED:
      return remoting::protocol::
          FileTransferResponse_ErrorCode_PERMISSIONS_ERROR;
    case base::File::FILE_ERROR_NO_SPACE:
      return remoting::protocol::
          FileTransferResponse_ErrorCode_OUT_OF_DISK_SPACE;
    default:
      return remoting::protocol::FileTransferResponse_ErrorCode_FILE_IO_ERROR;
  }
}

}  // namespace

namespace remoting {

class FileProxyWrapperLinux : public FileProxyWrapper {
 public:
  FileProxyWrapperLinux();
  ~FileProxyWrapperLinux() override;

  // FileProxyWrapper implementation.
  void Init(const ErrorCallback& error_callback) override;
  void CreateFile(const base::FilePath& directory,
                  const std::string& filename,
                  const SuccessCallback& success_callback) override;
  void WriteChunk(std::unique_ptr<CompoundBuffer> buffer) override;
  void Close(const SuccessCallback& success_callback) override;
  void Cancel() override;
  State state() override;

 private:
  struct WriteRequest {
    int64_t write_offset;
    std::vector<char> data;
  };

  // Callback for CreateFile().
  void CreateFileCallback(SuccessCallback success_callback,
                          base::File::Error error);

  // Callbacks for WriteChunk().
  void ConsumeWriteRequest(std::unique_ptr<WriteRequest> request);
  void WriteChunkCallback(base::File::Error error, int bytes_written);

  // Callbacks for Close().
  void CloseFileAndMoveToDestination();
  void CloseCallback(base::File::Error error);
  void PathExistsCallback(bool exists);
  void MoveFileCallback(bool success);

  void CancelWithError(protocol::FileTransferResponse_ErrorCode error);
  void SetState(State state);

  State state_ = kUninitialized;
  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;
  std::unique_ptr<base::FileProxy> file_proxy_;

  // Running this callback will not immediately destroy this class.
  ErrorCallback error_callback_;

  base::FilePath temp_filepath_;
  base::FilePath destination_filepath_;

  int64_t next_write_file_offset_ = 0;
  std::queue<std::unique_ptr<WriteRequest>> write_requests_;
  std::unique_ptr<WriteRequest> active_write_request_;

  SuccessCallback close_success_callback_;

  base::ThreadChecker thread_checker_;
  base::WeakPtrFactory<FileProxyWrapperLinux> weak_factory_;
};

FileProxyWrapperLinux::FileProxyWrapperLinux() : weak_factory_(this) {
  DCHECK(thread_checker_.CalledOnValidThread());
}

FileProxyWrapperLinux::~FileProxyWrapperLinux() = default;

void FileProxyWrapperLinux::Init(const ErrorCallback& error_callback) {
  DCHECK(thread_checker_.CalledOnValidThread());

  SetState(kInitialized);
  error_callback_ = error_callback;

  file_task_runner_ = base::CreateSequencedTaskRunnerWithTraits(
      {base::MayBlock(), base::TaskPriority::BACKGROUND});

  if (!file_task_runner_) {
    CancelWithError(protocol::FileTransferResponse_ErrorCode_FILE_IO_ERROR);
  }

  file_proxy_.reset(new base::FileProxy(file_task_runner_.get()));
}

void FileProxyWrapperLinux::CreateFile(
    const base::FilePath& directory,
    const std::string& filename,
    const SuccessCallback& success_callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK_EQ(state_, kInitialized);

  destination_filepath_ = directory.Append(filename);
  temp_filepath_ = destination_filepath_.AddExtension(kTempFileExtension);

  if (!file_proxy_->CreateOrOpen(
          temp_filepath_, base::File::FLAG_CREATE | base::File::FLAG_WRITE,
          base::Bind(&FileProxyWrapperLinux::CreateFileCallback,
                     weak_factory_.GetWeakPtr(), success_callback))) {
    // file_proxy_ failed to post a task to file_task_runner_.
    CancelWithError(protocol::FileTransferResponse_ErrorCode_FILE_IO_ERROR);
  }
}

void FileProxyWrapperLinux::CreateFileCallback(SuccessCallback success_callback,
                                               base::File::Error error) {
  if (error) {
    if (error == base::File::FILE_ERROR_EXISTS) {
      // TODO(jarhar): Try opening the temp file with another name instead of
      // failing.
    }
    CancelWithError(FileErrorToResponseError(error));
  } else {
    SetState(kFileCreated);
    success_callback.Run();
  }
}

void FileProxyWrapperLinux::WriteChunk(std::unique_ptr<CompoundBuffer> buffer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK_EQ(state_, kFileCreated);

  std::unique_ptr<WriteRequest> new_write_request =
      base::WrapUnique(new WriteRequest());
  new_write_request->data.resize(buffer->total_bytes());
  buffer->CopyTo(new_write_request->data.data(),
                 new_write_request->data.size());

  new_write_request->write_offset = next_write_file_offset_;
  next_write_file_offset_ += new_write_request->data.size();

  if (active_write_request_) {
    // TODO(jarhar): When flow control enabled QUIC-based WebRTC data channels
    // are implemented, block the flow of incoming chunks here if
    // write_requests_ has reached a maximum size. This implementation will
    // allow write_requests_ to grow without limits.
    write_requests_.push(std::move(new_write_request));
  } else {
    ConsumeWriteRequest(std::move(new_write_request));
  }
}

void FileProxyWrapperLinux::ConsumeWriteRequest(
    std::unique_ptr<WriteRequest> request) {
  active_write_request_ = std::move(request);
  if (!file_proxy_->Write(active_write_request_->write_offset,
                          active_write_request_->data.data(),
                          active_write_request_->data.size(),
                          base::Bind(&FileProxyWrapperLinux::WriteChunkCallback,
                                     weak_factory_.GetWeakPtr()))) {
    CancelWithError(protocol::FileTransferResponse_ErrorCode_FILE_IO_ERROR);
  }
}

void FileProxyWrapperLinux::WriteChunkCallback(base::File::Error error,
                                               int bytes_written) {
  if (error) {
    CancelWithError(FileErrorToResponseError(error));
    return;
  }

  active_write_request_.reset();
  if (!write_requests_.empty()) {
    std::unique_ptr<WriteRequest> request_to_consume =
        std::move(write_requests_.front());
    write_requests_.pop();
    ConsumeWriteRequest(std::move(request_to_consume));
  } else if (state_ == kClosing) {
    // All writes are complete and we have gotten the signal to move the file.
    CloseFileAndMoveToDestination();
  }
}

void FileProxyWrapperLinux::Close(const SuccessCallback& success_callback) {
  DCHECK(thread_checker_.CalledOnValidThread());

  SetState(kClosing);
  close_success_callback_ = success_callback;

  if (!active_write_request_ && write_requests_.empty()) {
    // All writes are complete, so we can finish up now.
    CloseFileAndMoveToDestination();
  }
}

void FileProxyWrapperLinux::CloseFileAndMoveToDestination() {
  DCHECK_EQ(state_, kClosing);
  file_proxy_->Close(base::Bind(&FileProxyWrapperLinux::CloseCallback,
                                weak_factory_.GetWeakPtr()));
}

void FileProxyWrapperLinux::CloseCallback(base::File::Error error) {
  if (error) {
    CancelWithError(FileErrorToResponseError(error));
    return;
  }

  PostTaskAndReplyWithResult(
      file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&base::PathExists, destination_filepath_),
      base::BindOnce(&FileProxyWrapperLinux::PathExistsCallback,
                     weak_factory_.GetWeakPtr()));
}

void FileProxyWrapperLinux::PathExistsCallback(bool exists) {
  if (exists) {
    // TODO(jarhar): Change the destination filepath and try again instead of
    // failing.
    CancelWithError(protocol::FileTransferResponse_ErrorCode_FILE_IO_ERROR);
  } else {
    PostTaskAndReplyWithResult(
        file_task_runner_.get(), FROM_HERE,
        base::BindOnce(&base::Move, temp_filepath_, destination_filepath_),
        base::BindOnce(&FileProxyWrapperLinux::MoveFileCallback,
                       weak_factory_.GetWeakPtr()));
  }
}

void FileProxyWrapperLinux::MoveFileCallback(bool success) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (success) {
    SetState(kCompleted);
    close_success_callback_.Run();
  } else {
    CancelWithError(protocol::FileTransferResponse_ErrorCode_FILE_IO_ERROR);
  }
}

void FileProxyWrapperLinux::Cancel() {
  if (file_proxy_->IsValid()) {
    file_proxy_->Close(base::Bind(&EmptyStatusCallback));
  }

  if (state_ == kFileCreated || state_ == kClosing) {
    PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
                               base::BindOnce(&base::DeleteFile, temp_filepath_,
                                              false /* recursive */),
                               base::BindOnce(&EmptyDeleteFileCallback));
  }

  if (state_ == kClosing || state_ == kCompleted) {
    PostTaskAndReplyWithResult(
        file_task_runner_.get(), FROM_HERE,
        base::BindOnce(&base::DeleteFile, destination_filepath_,
                       false /* recursive */),
        base::BindOnce(&EmptyDeleteFileCallback));
  }

  SetState(kCancelled);
}

void FileProxyWrapperLinux::CancelWithError(
    protocol::FileTransferResponse_ErrorCode error) {
  Cancel();
  error_callback_.Run(error);
}

void FileProxyWrapperLinux::SetState(State state) {
  switch (state) {
    case kUninitialized:
      // No state can change to kUninitialized.
      DCHECK_EQ(state_, kUninitialized);
      break;
    case kInitialized:
      DCHECK_EQ(state_, kUninitialized);
      break;
    case kFileCreated:
      DCHECK_EQ(state_, kInitialized);
      break;
    case kClosing:
      DCHECK_EQ(state_, kFileCreated);
      break;
    case kCompleted:
      DCHECK_EQ(state_, kClosing);
      break;
    case kCancelled:
      // Any state can change to kCancelled.
      break;
  }

  state_ = state;
}

FileProxyWrapper::State FileProxyWrapperLinux::state() {
  return state_;
}

// static
std::unique_ptr<FileProxyWrapper> FileProxyWrapper::Create() {
  return base::WrapUnique(new FileProxyWrapperLinux());
}

}  // namespace remoting
