// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mojo_cdm_file_io.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/task_scheduler/post_task.h"

namespace media {

namespace {

// File size limit is 32MB. Licenses saved by the CDM are typically several
// hundreds of bytes.
const int64_t kFileSizeLimit = 32 * 1024 * 1024;

// Maximum length of a file name.
const size_t kFileNameMaxLength = 256;

cdm::FileIOClient::Status ConvertStatus(mojom::CdmStorage::Status status) {
  switch (status) {
    case mojom::CdmStorage::Status::SUCCESS:
      return cdm::FileIOClient::Status::kSuccess;
    case mojom::CdmStorage::Status::IN_USE:
      return cdm::FileIOClient::Status::kInUse;
    case mojom::CdmStorage::Status::FAILURE:
      return cdm::FileIOClient::Status::kError;
  }

  NOTREACHED() << "Unrecognized status " << status;
  return cdm::FileIOClient::Status::kError;
}

// File names must only contain letters (A-Za-z), digits(0-9), or "._-",
// and not start with "_". It must contain at least 1 character, and not
// more then |kFileNameMaxLength| characters.
static bool IsValidFileName(const std::string& name) {
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

bool MojoCdmFileIO::StateManager::SetState(State op) {
  DVLOG(3) << __func__ << " from " << (int)current_state_ << " to " << (int)op;
  switch (op) {
    case State::kNone:
      // Not allowed.
      return false;
    case State::kOpening:
      if (current_state_ != State::kNone)
        return false;
      break;
    case State::kOpened:
      DCHECK(current_state_ == State::kOpening ||
             current_state_ == State::kReading ||
             current_state_ == State::kWriting);
      break;
    case State::kReading:
      if (current_state_ != State::kOpened)
        return false;
      break;
    case State::kWriting:
      if (current_state_ != State::kOpened)
        return false;
      break;
  }
  current_state_ = op;
  return true;
}

MojoCdmFileIO::MojoCdmFileIO(cdm::FileIOClient* client, OpenCB open_callback)
    : client_(client),
      open_callback_(std::move(open_callback)),
      weak_factory_(this) {
  DVLOG(3) << __func__;
  DCHECK(client_);
  DCHECK(open_callback_);
}

MojoCdmFileIO::~MojoCdmFileIO() {
  // The destructor is private. |this| can only be destructed through Close().
}

void MojoCdmFileIO::Open(const char* file_name, uint32_t file_name_size) {
  std::string name(file_name, file_name_size);
  DVLOG(3) << __func__ << " file: " << name;

  if (!IsValidFileName(name) ||
      !state_manager_.SetState(StateManager::State::kOpening)) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&MojoCdmFileIO::OnOpenComplete,
                                  weak_factory_.GetWeakPtr(),
                                  mojom::CdmStorage::Status::FAILURE,
                                  base::File(), nullptr));
    return;
  }

  file_name_ = name;
  std::move(open_callback_)
      .Run(file_name_, base::BindOnce(&MojoCdmFileIO::OnOpenComplete,
                                      weak_factory_.GetWeakPtr()));
}

void MojoCdmFileIO::OnOpenComplete(
    mojom::CdmStorage::Status status,
    base::File file,
    mojom::CdmFileReleaserAssociatedPtrInfo cdm_file_releaser) {
  DVLOG(3) << __func__ << " file: " << file_name_ << ", status: " << status;

  if (status == mojom::CdmStorage::Status::SUCCESS) {
    state_manager_.SetState(StateManager::State::kOpened);
    file_ = std::move(file);
    cdm_file_releaser_.Bind(std::move(cdm_file_releaser));
  }

  client_->OnOpenComplete(ConvertStatus(status));
}

void MojoCdmFileIO::Read() {
  DVLOG(3) << __func__ << " file: " << file_name_;

  if (!state_manager_.SetState(StateManager::State::kReading)) {
    cdm::FileIOClient::Status status = state_manager_.IsReadOrWriteInProgress()
                                           ? cdm::FileIOClient::Status::kInUse
                                           : cdm::FileIOClient::Status::kError;
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&MojoCdmFileIO::OnReadComplete,
                       weak_factory_.GetWeakPtr(), status, nullptr, 0));
    return;
  }

  // Now that we have set the state to do the read, run it asynchronously so
  // that we don't need to copy the data when calling |client_|.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&MojoCdmFileIO::DoReadAsync, weak_factory_.GetWeakPtr()));
}

void MojoCdmFileIO::DoReadAsync() {
  if (!file_.IsValid()) {
    OnReadComplete(cdm::FileIOClient::Status::kError, nullptr, 0);
    return;
  }

  // Determine the size of the file (so we know how many bytes to read).
  // If the file has 0 bytes, no need to read anything.
  int64_t num_bytes = file_.GetLength();
  if (num_bytes < 0) {
    // An error occurred, so fail.
    OnReadComplete(cdm::FileIOClient::Status::kError, nullptr, 0);
    return;
  }
  if (num_bytes == 0) {
    OnReadComplete(cdm::FileIOClient::Status::kSuccess, nullptr, 0);
    return;
  }

  // Files are limited to 32MB, so fail if file too big.
  if (num_bytes > kFileSizeLimit) {
    DLOG(WARNING) << __func__
                  << " Too much data to read. #bytes = " << num_bytes;
    OnReadComplete(cdm::FileIOClient::Status::kError, nullptr, 0);
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
    OnReadComplete(cdm::FileIOClient::Status::kError, nullptr, 0);
    return;
  }

  OnReadComplete(cdm::FileIOClient::Status::kSuccess, file_data.data(),
                 file_data.size());
}

void MojoCdmFileIO::OnReadComplete(cdm::FileIOClient::Status status,
                                   const uint8_t* data,
                                   uint32_t data_size) {
  DVLOG(3) << __func__ << " file: " << file_name_ << ", read: " << data_size
           << ", status: " << status;

  // Only reset the state if completing a read.
  if (state_manager_.GetState() == StateManager::State::kReading)
    state_manager_.SetState(StateManager::State::kOpened);

  client_->OnReadComplete(status, data, data_size);
}

void MojoCdmFileIO::Write(const uint8_t* data, uint32_t data_size) {
  DVLOG(3) << __func__ << " file: " << file_name_ << ", bytes: " << data_size;

  if (!state_manager_.SetState(StateManager::State::kWriting)) {
    cdm::FileIOClient::Status status = state_manager_.IsReadOrWriteInProgress()
                                           ? cdm::FileIOClient::Status::kInUse
                                           : cdm::FileIOClient::Status::kError;
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&MojoCdmFileIO::OnWriteCompleteAsync,
                                  weak_factory_.GetWeakPtr(), status));
    return;
  }

  if (!file_.IsValid()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&MojoCdmFileIO::OnWriteCompleteAsync,
                                  weak_factory_.GetWeakPtr(),
                                  cdm::FileIOClient::Status::kError));
    return;
  }

  // Files are limited to 32MB, so fail if file too big.
  if (data_size > kFileSizeLimit) {
    DLOG(WARNING) << __func__
                  << " Too much data to write. #bytes = " << data_size;
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&MojoCdmFileIO::OnWriteCompleteAsync,
                                  weak_factory_.GetWeakPtr(),
                                  cdm::FileIOClient::Status::kError));
    return;
  }

  // Now write the data to the file (if there is data).
  cdm::FileIOClient::Status status = cdm::FileIOClient::Status::kSuccess;
  int bytes_to_write = base::checked_cast<int>(data_size);
  if (bytes_to_write > 0) {
    int bytes_written =
        file_.Write(0, reinterpret_cast<const char*>(data), bytes_to_write);
    if (bytes_written != bytes_to_write) {
      // Failed to write what was requested, so try to clear the file and fail.
      status = cdm::FileIOClient::Status::kError;
      bytes_to_write = 0;
    }
  }

  if (!file_.SetLength(bytes_to_write))
    status = cdm::FileIOClient::Status::kError;

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&MojoCdmFileIO::OnWriteCompleteAsync,
                                weak_factory_.GetWeakPtr(), status));
}

void MojoCdmFileIO::OnWriteCompleteAsync(cdm::FileIOClient::Status status) {
  DVLOG(3) << __func__ << " file: " << file_name_ << ", status: " << status;

  // Only reset the state if completing a write.
  if (state_manager_.GetState() == StateManager::State::kWriting)
    state_manager_.SetState(StateManager::State::kOpened);

  client_->OnWriteComplete(status);
}

void MojoCdmFileIO::Close() {
  DVLOG(3) << __func__ << " file: " << file_name_;
  delete this;
}

}  // namespace media
