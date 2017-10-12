// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mojo_cdm_file_io.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/task_scheduler/post_task.h"
#include "media/base/scoped_callback_runner.h"

namespace media {

namespace {

using ClientStatus = cdm::FileIOClient::Status;
using StorageStatus = media::mojom::CdmStorage::Status;

// File size limit is 32MB. Licenses saved by the CDM are typically several
// hundreds of bytes.
const int64_t kMaxFileSizeBytes = 32 * 1024 * 1024;

// Maximum length of a file name.
const size_t kFileNameMaxLength = 256;

// File names must only contain letters (A-Za-z), digits(0-9), or "._-",
// and not start with "_". It must contain at least 1 character, and not
// more then |kFileNameMaxLength| characters.
bool IsValidFileName(const std::string& name) {
  if (name.empty() || name.length() > kFileNameMaxLength || name[0] == '_')
    return false;

  for (const auto& ch : name) {
    if (!base::IsAsciiAlpha(ch) && !base::IsAsciiDigit(ch) && ch != '.' &&
        ch != '_' && ch != '-') {
      return false;
    }
  }

  return true;
}

}  // namespace

MojoCdmFileIO::MojoCdmFileIO(cdm::FileIOClient* client,
                             mojom::CdmStorage* cdm_storage)
    : client_(client), cdm_storage_(cdm_storage), weak_factory_(this) {
  DVLOG(3) << __func__;
  DCHECK(client_);
  DCHECK(cdm_storage_);
}

MojoCdmFileIO::~MojoCdmFileIO() {
  // The destructor is private. |this| can only be destructed through Close().
}

void MojoCdmFileIO::Open(const char* file_name, uint32_t file_name_size) {
  std::string name(file_name, file_name_size);
  DVLOG(3) << __func__ << " file: " << name;

  // Open is only allowed if the current state is kUnopened and the file name
  // is valid. Result is passed to DoOpen() so that |client_| can be notified
  // asynchronously.
  bool can_open = false;
  if (state_ == State::kUnopened && IsValidFileName(name)) {
    file_name_ = base::FilePath().AppendASCII(name);
    state_ = State::kOpening;
    can_open = true;
  }

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&MojoCdmFileIO::DoOpen,
                                weak_factory_.GetWeakPtr(), name, can_open));
}

void MojoCdmFileIO::DoOpen(const std::string& file_name, bool can_open) {
  // If we failed to switch into state kOpening or some validation failed,
  // don't proceed.
  if (!can_open) {
    OnFileOpened(StorageStatus::kFailure, base::File(), nullptr);
    return;
  }

  auto callback = ScopedCallbackRunner(
      base::BindOnce(&MojoCdmFileIO::OnFileOpened, weak_factory_.GetWeakPtr()),
      StorageStatus::kFailure, base::File(), nullptr);
  cdm_storage_->Open(file_name, std::move(callback));
}

void MojoCdmFileIO::OnFileOpened(
    StorageStatus status,
    base::File file,
    mojom::CdmFileReleaserAssociatedPtrInfo cdm_file_releaser) {
  DVLOG(3) << __func__ << " file: " << file_name_ << ", status: " << status;

  ClientStatus client_status = ClientStatus::kError;
  switch (status) {
    case StorageStatus::kSuccess:
      // File was successfully opened.
      state_ = State::kOpened;
      file_ = std::move(file);
      cdm_file_releaser_.Bind(std::move(cdm_file_releaser));
      client_status = ClientStatus::kSuccess;
      break;
    case StorageStatus::kInUse:
      // File already open by somebody else.
      state_ = State::kUnopened;
      client_status = ClientStatus::kInUse;
      break;
    case StorageStatus::kFailure:
      // Something went wrong.
      state_ = State::kError;
      client_status = ClientStatus::kError;
      break;
  }

  client_->OnOpenComplete(client_status);
}

void MojoCdmFileIO::Read() {
  DVLOG(3) << __func__ << " file: " << file_name_;

  // Do the actual read asynchronously so that we don't need to copy the
  // data when calling |client_|.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&MojoCdmFileIO::DoRead, weak_factory_.GetWeakPtr()));
}

void MojoCdmFileIO::DoRead() {
  DVLOG(3) << __func__ << " file: " << file_name_;

  // Reading a file requires getting the length first to know how much data
  // is available, then reading the complete contents of the file into a
  // buffer and passing it back to |client_|. As these should be small files,
  // we don't worry about breaking it up into chunks to read it.

  // If the file hasn't been opened yet, we can't proceed. This also covers
  // open in progress and error states.
  if (state_ != State::kOpened) {
    client_->OnReadComplete(ClientStatus::kError, nullptr, 0);
    return;
  }

  if (!file_.IsValid()) {
    // This error is not recoverable, so don't allow any more operations
    // other than close on the file.
    state_ = State::kError;
    client_->OnReadComplete(ClientStatus::kError, nullptr, 0);
    return;
  }

  // Determine the size of the file (so we know how many bytes to read).
  // If the file has 0 bytes, no need to read anything.
  int64_t num_bytes = file_.GetLength();
  if (num_bytes < 0) {
    // Negative bytes mean failure, so fail. This error is not recoverable,
    // so don't allow any more operations other than close on the file.
    state_ = State::kError;
    client_->OnReadComplete(ClientStatus::kError, nullptr, 0);
    return;
  }
  if (num_bytes == 0) {
    client_->OnReadComplete(ClientStatus::kSuccess, nullptr, 0);
    return;
  }

  // Files are limited to 32MB, so fail if file too big.
  if (num_bytes > kMaxFileSizeBytes) {
    DLOG(WARNING) << __func__
                  << " Too much data to read. #bytes = " << num_bytes;
    client_->OnReadComplete(ClientStatus::kError, nullptr, 0);
    return;
  }

  // Read the contents of the file. Read() sizes (provided and returned) are
  // type int, so cast appropriately.
  int bytes_to_read = base::checked_cast<int>(num_bytes);
  std::vector<uint8_t> file_data(bytes_to_read);
  int bytes_read =
      file_.Read(0, reinterpret_cast<char*>(file_data.data()), bytes_to_read);
  if (bytes_to_read != bytes_read) {
    DVLOG(1) << "Failed to read file " << file_name_ << ". Requested "
             << bytes_to_read << " bytes, got " << bytes_read;
    client_->OnReadComplete(ClientStatus::kError, nullptr, 0);
    return;
  }

  client_->OnReadComplete(ClientStatus::kSuccess, file_data.data(),
                          file_data.size());
}

void MojoCdmFileIO::Write(const uint8_t* data, uint32_t data_size) {
  DVLOG(3) << __func__ << " file: " << file_name_ << ", bytes: " << data_size;

  // Writing a file requires writing the full data provided into the file and
  // then setting the length so that any previous (longer) data is truncated.
  // As the size is small, the file is written in one chunk rather than
  // multiple pieces.

  // If the file is not open, fail.
  if (state_ != State::kOpened) {
    PostWriteResult(ClientStatus::kError);
    return;
  }

  if (!file_.IsValid()) {
    // This error is not recoverable, so don't allow any more operations
    // other than close on the file.
    state_ = State::kError;
    PostWriteResult(ClientStatus::kError);
    return;
  }

  // Files are limited to 32MB, so fail if file too big.
  if (data_size > kMaxFileSizeBytes) {
    DLOG(WARNING) << __func__
                  << " Too much data to write. #bytes = " << data_size;
    PostWriteResult(ClientStatus::kError);
    return;
  }

  // Now write the data to the file (if there is data).
  ClientStatus status = ClientStatus::kSuccess;
  int bytes_to_write = base::checked_cast<int>(data_size);
  if (bytes_to_write > 0) {
    int bytes_written =
        file_.Write(0, reinterpret_cast<const char*>(data), bytes_to_write);
    DVLOG(3) << __func__ << " write: " << bytes_to_write
             << ", written: " << bytes_written;
    if (bytes_written != bytes_to_write) {
      // Failed to write what was requested, so try to clear the file and fail.
      status = ClientStatus::kError;
      bytes_to_write = 0;
    }
  }

  if (!file_.SetLength(bytes_to_write)) {
    // If this fails don't allow any more operations other than close
    // on the file.
    state_ = State::kError;
    status = ClientStatus::kError;
  }

  PostWriteResult(status);
}

void MojoCdmFileIO::PostWriteResult(ClientStatus status) {
  DVLOG(3) << __func__ << " file: " << file_name_ << ", status: " << status;

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&MojoCdmFileIO::NotifyClientOfWriteResult,
                                weak_factory_.GetWeakPtr(), status));
}

void MojoCdmFileIO::NotifyClientOfWriteResult(ClientStatus status) {
  DVLOG(3) << __func__ << " file: " << file_name_ << ", status: " << status;

  client_->OnWriteComplete(status);
}

void MojoCdmFileIO::Close() {
  DVLOG(3) << __func__ << " file: " << file_name_;
  delete this;
}

}  // namespace media
