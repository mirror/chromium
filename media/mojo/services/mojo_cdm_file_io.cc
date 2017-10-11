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

using CdmStatus = cdm::FileIOClient::Status;
using MojoStatus = media::mojom::CdmStorage::Status;

namespace media {

namespace {

// File size limit is 32MB. Licenses saved by the CDM are typically several
// hundreds of bytes.
const int64_t kMaxFileSizeBytes = 32 * 1024 * 1024;

// Maximum length of a file name.
const size_t kFileNameMaxLength = 256;

CdmStatus ConvertStatus(MojoStatus status) {
  switch (status) {
    case MojoStatus::SUCCESS:
      return CdmStatus::kSuccess;
    case MojoStatus::IN_USE:
      return CdmStatus::kInUse;
    case MojoStatus::FAILURE:
      return CdmStatus::kError;
  }

  NOTREACHED() << "Unrecognized status " << status;
  return CdmStatus::kError;
}

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

  // Open is only allowed if the current state is kNone and the file name
  // is valid. Result is passed to DoOpen() so that |client_| can be notified
  // asynchronously.
  bool can_open = false;
  if (state_ == State::kNone && IsValidFileName(name)) {
    file_name_ = base::FilePath(name);
    state_ = State::kOpening;
    can_open = true;
  }

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&MojoCdmFileIO::DoOpen,
                                weak_factory_.GetWeakPtr(), name, can_open));
}

void MojoCdmFileIO::DoOpen(const std::string& file_name, bool can_open) {
  mojom::CdmStorage::OpenCallback callback = ScopedCallbackRunner(
      base::BindOnce(&MojoCdmFileIO::NotifyClientOfOpenResult,
                     weak_factory_.GetWeakPtr()),
      mojom::CdmStorage::Status::FAILURE, base::File(), nullptr);

  // If we failed to switch into state kOpening or some validation failed,
  // don't proceed.
  if (!can_open)
    return;

  cdm_storage_->Open(file_name, std::move(callback));
}

void MojoCdmFileIO::NotifyClientOfOpenResult(
    MojoStatus status,
    base::File file,
    mojom::CdmFileReleaserAssociatedPtrInfo cdm_file_releaser) {
  DVLOG(3) << __func__ << " file: " << file_name_ << ", status: " << status;

  // If unsuccessful, |state_| will be left in kOpening and the only thing
  // possible is calling Close().
  if (status == MojoStatus::SUCCESS) {
    state_ = State::kOpened;
    file_ = std::move(file);
    cdm_file_releaser_.Bind(std::move(cdm_file_releaser));
  }

  client_->OnOpenComplete(ConvertStatus(status));
}

void MojoCdmFileIO::Read() {
  DVLOG(3) << __func__ << " file: " << file_name_;

  // Try to switch into kReading state. DoRead() will check whether it
  // succeeded or not so that the client can be notified asynchronously.
  CdmStatus set_read_state = SwitchStateForReadingOrWriting(State::kReading);

  // Now that we have set the state to do the read, run it asynchronously so
  // that we don't need to copy the data when calling |client_|.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&MojoCdmFileIO::DoRead,
                                weak_factory_.GetWeakPtr(), set_read_state));
}

void MojoCdmFileIO::DoRead(CdmStatus set_read_state) {
  DVLOG(3) << __func__ << " file: " << file_name_;

  // Reading a file requires getting the length first to know how much data
  // is available, then reading the complete contents of the file into a
  // buffer and passing it back to |client_|. As these should be small files,
  // we don't worry about breaking it up into chunks to read it.

  // If we failed to switch into state kReading, we can't proceed.
  if (set_read_state != CdmStatus::kSuccess) {
    NotifyClientOfReadFailure(set_read_state, false);
    return;
  }

  if (!file_.IsValid()) {
    NotifyClientOfReadFailure(CdmStatus::kError, true);
    return;
  }

  // Determine the size of the file (so we know how many bytes to read).
  // If the file has 0 bytes, no need to read anything.
  int64_t num_bytes = file_.GetLength();
  if (num_bytes < 0) {
    // Negative bytes mean failure, so fail.
    NotifyClientOfReadFailure(CdmStatus::kError, true);
    return;
  }
  if (num_bytes == 0) {
    NotifyClientOfReadSuccess(nullptr, 0);
    return;
  }

  // Files are limited to 32MB, so fail if file too big.
  if (num_bytes > kMaxFileSizeBytes) {
    DLOG(WARNING) << __func__
                  << " Too much data to read. #bytes = " << num_bytes;
    NotifyClientOfReadFailure(CdmStatus::kError, true);
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
    NotifyClientOfReadFailure(CdmStatus::kError, true);
    return;
  }

  NotifyClientOfReadSuccess(file_data.data(), file_data.size());
}

void MojoCdmFileIO::NotifyClientOfReadSuccess(const uint8_t* data,
                                              uint32_t data_size) {
  DVLOG(3) << __func__ << " file: " << file_name_ << ", read: " << data_size;

  // |state_| must have been successfully changed or nothing would be read.
  DCHECK_EQ(State::kReading, state_);
  state_ = State::kOpened;
  client_->OnReadComplete(CdmStatus::kSuccess, data, data_size);
}

void MojoCdmFileIO::NotifyClientOfReadFailure(cdm::FileIOClient::Status status,
                                              bool reset_state) {
  DVLOG(3) << __func__ << " file: " << file_name_ << ", status: " << status;

  // Only reset the state if state was changed successfully.
  if (reset_state) {
    DCHECK_EQ(State::kReading, state_);
    state_ = State::kOpened;
  }

  client_->OnReadComplete(status, nullptr, 0);
}

void MojoCdmFileIO::Write(const uint8_t* data, uint32_t data_size) {
  DVLOG(3) << __func__ << " file: " << file_name_ << ", bytes: " << data_size;

  // Writing a file requires writing the full data provided into the file and
  // then setting the length so that any previous (longer) data is truncated.
  // As the size is small, the file is written in one chunk rather than
  // multiple pieces.

  // Try to switch into kWriting state. If we failed to switch into state
  // kWriting, we can't proceed.
  CdmStatus set_write_state = SwitchStateForReadingOrWriting(State::kWriting);
  if (set_write_state != CdmStatus::kSuccess) {
    ReportWriteResult(set_write_state, false);
    return;
  }

  if (!file_.IsValid()) {
    ReportWriteResult(CdmStatus::kError, true);
    return;
  }

  // Files are limited to 32MB, so fail if file too big.
  if (data_size > kMaxFileSizeBytes) {
    DLOG(WARNING) << __func__
                  << " Too much data to write. #bytes = " << data_size;
    ReportWriteResult(CdmStatus::kError, true);
    return;
  }

  // Now write the data to the file (if there is data).
  CdmStatus status = CdmStatus::kSuccess;
  int bytes_to_write = base::checked_cast<int>(data_size);
  if (bytes_to_write > 0) {
    int bytes_written =
        file_.Write(0, reinterpret_cast<const char*>(data), bytes_to_write);
    if (bytes_written != bytes_to_write) {
      // Failed to write what was requested, so try to clear the file and fail.
      status = CdmStatus::kError;
      bytes_to_write = 0;
    }
  }

  if (!file_.SetLength(bytes_to_write))
    status = CdmStatus::kError;

  ReportWriteResult(status, true);
}

void MojoCdmFileIO::ReportWriteResult(CdmStatus status, bool reset_state) {
  DVLOG(3) << __func__ << " file: " << file_name_ << ", status: " << status;

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&MojoCdmFileIO::NotifyClientOfWriteResult,
                     weak_factory_.GetWeakPtr(), status, reset_state));
}

void MojoCdmFileIO::NotifyClientOfWriteResult(CdmStatus status,
                                              bool reset_state) {
  DVLOG(3) << __func__ << " file: " << file_name_ << ", status: " << status;

  // Only reset the state if completing a write.
  if (reset_state) {
    DCHECK_EQ(State::kWriting, state_);
    state_ = State::kOpened;
  }

  client_->OnWriteComplete(status);
}

void MojoCdmFileIO::Close() {
  DVLOG(3) << __func__ << " file: " << file_name_;
  delete this;
}

CdmStatus MojoCdmFileIO::SwitchStateForReadingOrWriting(State new_state) {
  DCHECK(new_state == State::kReading || new_state == State::kWriting);

  // Read or Write is only allowed if the current state is kOpened.
  if (state_ == State::kOpened) {
    state_ = new_state;
    return CdmStatus::kSuccess;
  }

  // If there is already a read/write operation in progress, return kInUse.
  if (state_ == State::kReading || state_ == State::kWriting)
    return CdmStatus::kInUse;

  // Can't do the read or write operation in the current state.
  return CdmStatus::kError;
}

}  // namespace media
