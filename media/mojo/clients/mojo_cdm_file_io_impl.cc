// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/clients/mojo_cdm_file_io_impl.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file.h"
#include "base/task_scheduler/post_task.h"

namespace media {

namespace {

// File size limit is 32MB. Licenses saved by the CDM are typically several
// hundreds of bytes.
const int64_t kFileSizeLimit = 32 * 1024 * 1024;

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

}  // namespace

bool MojoCdmFileIOImpl::StateManager::SetState(OperationType op) {
  switch (op) {
    case OperationType::kNone:
      // Not allowed.
      return false;
    case OperationType::kOpening:
      if (current_state_ != OperationType::kNone)
        return false;
      break;
    case OperationType::kOpened:
      DCHECK(current_state_ == OperationType::kOpening ||
             current_state_ == OperationType::kRead ||
             current_state_ == OperationType::kWrite);
      break;
    case OperationType::kRead:
      if (current_state_ != OperationType::kOpened)
        return false;
      break;
    case OperationType::kWrite:
      if (current_state_ != OperationType::kOpened)
        return false;
      break;
    case OperationType::kClosed:
      // always allowed.
      break;
  }
  current_state_ = op;
  return true;
}

MojoCdmFileIOImpl::MojoCdmFileIOImpl(const std::string& key_system,
                                     mojom::CdmStoragePtr storage,
                                     cdm::FileIOClient* client)
    : key_system_(key_system),
      storage_(std::move(storage)),
      client_(client),
      file_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
      file_(file_task_runner_.get()),
      weak_factory_(this) {
  DCHECK(storage_);
  DVLOG(3) << __func__;
}

MojoCdmFileIOImpl::~MojoCdmFileIOImpl() {
  // The destructor is private. |this| can only be destructed through Close().
  DCHECK_EQ(StateManager::OperationType::kClosed, state_manager_.GetState());
}

void MojoCdmFileIOImpl::Open(const char* file_name, uint32_t file_name_size) {
  std::string name(file_name, file_name_size);
  DVLOG(3) << __func__ << " file: " << name;

  if (!state_manager_.SetState(StateManager::OperationType::kOpening)) {
    PostTask(base::BindOnce(&MojoCdmFileIOImpl::OnOpenComplete,
                            weak_factory_.GetWeakPtr(),
                            mojom::CdmStorage::Status::FAILURE, base::File()));
    return;
  }

  file_name_.swap(name);
  storage_->Initialize(key_system_,
                       base::BindOnce(&MojoCdmFileIOImpl::OnInitializeComplete,
                                      weak_factory_.GetWeakPtr()));
}

void MojoCdmFileIOImpl::OnInitializeComplete(bool status) {
  DVLOG(3) << __func__ << " file: " << file_name_
           << (status ? " success" : " failure");

  if (!status) {
    // State will be left in OPENING.
    client_->OnOpenComplete(cdm::FileIOClient::Status::kError);
    return;
  }

  storage_->Open(file_name_, base::BindOnce(&MojoCdmFileIOImpl::OnOpenComplete,
                                            weak_factory_.GetWeakPtr()));
}

void MojoCdmFileIOImpl::OnOpenComplete(mojom::CdmStorage::Status status,
                                       base::File file) {
  DVLOG(3) << __func__ << " file: " << file_name_ << ", status: " << status;

  if (status == mojom::CdmStorage::Status::SUCCESS) {
    state_manager_.SetState(StateManager::OperationType::kOpened);
    file_.SetFile(std::move(file));
  }

  client_->OnOpenComplete(ConvertStatus(status));
}

void MojoCdmFileIOImpl::Read() {
  DVLOG(3) << __func__ << " file: " << file_name_;

  if (!state_manager_.SetState(StateManager::OperationType::kRead)) {
    cdm::FileIOClient::Status status = state_manager_.IsReadOrWriteInProgress()
                                           ? cdm::FileIOClient::Status::kInUse
                                           : cdm::FileIOClient::Status::kError;
    PostTask(base::BindOnce(&MojoCdmFileIOImpl::OnReadComplete,
                            weak_factory_.GetWeakPtr(), status, nullptr, 0));
    return;
  }

  if (file_.GetInfo(base::Bind(&MojoCdmFileIOImpl::OnGetInfoComplete,
                               weak_factory_.GetWeakPtr()))) {
    return;
  }

  NOTREACHED() << "GetInfo() failed.";
  PostTask(base::BindOnce(&MojoCdmFileIOImpl::OnReadComplete,
                          weak_factory_.GetWeakPtr(),
                          cdm::FileIOClient::Status::kError, nullptr, 0));
}

void MojoCdmFileIOImpl::OnGetInfoComplete(base::File::Error error,
                                          const base::File::Info& info) {
  DVLOG(3) << __func__ << " file: " << file_name_;
  if (error != base::File::FILE_OK) {
    OnReadComplete(cdm::FileIOClient::Status::kError, nullptr, 0);
    return;
  }

  if (info.size == 0) {
    OnReadComplete(cdm::FileIOClient::Status::kSuccess, nullptr, 0);
    return;
  }

  if (info.size > kFileSizeLimit) {
    DLOG(WARNING) << __func__
                  << " Too much data to read. #bytes = " << info.size;
    OnReadComplete(cdm::FileIOClient::Status::kError, nullptr, 0);
    return;
  }

  if (file_.Read(0, info.size,
                 base::Bind(&MojoCdmFileIOImpl::OnFileRead,
                            weak_factory_.GetWeakPtr()))) {
    return;
  }

  NOTREACHED() << "Read() failed.";
  OnReadComplete(cdm::FileIOClient::Status::kError, nullptr, 0);
}

void MojoCdmFileIOImpl::OnFileRead(base::File::Error error,
                                   const char* data,
                                   int bytes_read) {
  DVLOG(3) << __func__ << " file: " << file_name_;
  if (error != base::File::FILE_OK) {
    OnReadComplete(cdm::FileIOClient::Status::kError, nullptr, 0);
    return;
  }

  OnReadComplete(cdm::FileIOClient::Status::kSuccess,
                 reinterpret_cast<const uint8_t*>(data), bytes_read);
}

void MojoCdmFileIOImpl::OnReadComplete(cdm::FileIOClient::Status status,
                                       const uint8_t* data,
                                       uint32_t data_size) {
  DVLOG(3) << __func__ << " file: " << file_name_ << ", read: " << data_size
           << ", status: " << status;

  // Only reset the state if completing a read.
  if (state_manager_.GetState() == StateManager::OperationType::kRead)
    state_manager_.SetState(StateManager::OperationType::kOpened);

  client_->OnReadComplete(status, data, data_size);
}

void MojoCdmFileIOImpl::Write(const uint8_t* data, uint32_t data_size) {
  DVLOG(3) << __func__ << " file: " << file_name_ << ", bytes: " << data_size;

  if (!state_manager_.SetState(StateManager::OperationType::kWrite)) {
    cdm::FileIOClient::Status status = state_manager_.IsReadOrWriteInProgress()
                                           ? cdm::FileIOClient::Status::kInUse
                                           : cdm::FileIOClient::Status::kError;
    PostTask(base::BindOnce(&MojoCdmFileIOImpl::OnWriteComplete,
                            weak_factory_.GetWeakPtr(), status));
    return;
  }

  if (data_size > kFileSizeLimit) {
    DLOG(WARNING) << __func__
                  << " Too much data to write. #bytes = " << data_size;
    PostTask(base::BindOnce(&MojoCdmFileIOImpl::OnWriteComplete,
                            weak_factory_.GetWeakPtr(),
                            cdm::FileIOClient::Status::kError));
    return;
  }

  // FileProxy doesn't support Write() with 0 bytes, so simply set the length.
  if (data_size == 0) {
    if (!file_.SetLength(0, base::Bind(&MojoCdmFileIOImpl::OnFileLengthSet,
                                       weak_factory_.GetWeakPtr(),
                                       cdm::FileIOClient::Status::kSuccess))) {
      NOTREACHED() << "SetLength() failed.";
      PostTask(base::BindOnce(&MojoCdmFileIOImpl::OnWriteComplete,
                              weak_factory_.GetWeakPtr(),
                              cdm::FileIOClient::Status::kError));
    }
    return;
  }

  if (file_.Write(0, reinterpret_cast<const char*>(data), data_size,
                  base::Bind(&MojoCdmFileIOImpl::OnFileWritten,
                             weak_factory_.GetWeakPtr()))) {
    return;
  }

  NOTREACHED() << "Write() failed.";
  PostTask(base::BindOnce(&MojoCdmFileIOImpl::OnWriteComplete,
                          weak_factory_.GetWeakPtr(),
                          cdm::FileIOClient::Status::kError));
}

void MojoCdmFileIOImpl::OnFileWritten(base::File::Error error,
                                      int bytes_written) {
  DVLOG(3) << __func__ << " file: " << file_name_
           << ", written: " << bytes_written;

  // Now that the data is written, the length of the file needs to be set
  // properly.
  int file_length = bytes_written;
  cdm::FileIOClient::Status result = cdm::FileIOClient::Status::kSuccess;

  // If writing to the file failed, attempt to set the length to 0. |client_|
  // need to still get an error indicating that the write failed.
  if (error != base::File::FILE_OK) {
    file_length = 0;
    result = cdm::FileIOClient::Status::kError;
  }

  if (file_.SetLength(file_length,
                      base::Bind(&MojoCdmFileIOImpl::OnFileLengthSet,
                                 weak_factory_.GetWeakPtr(), result))) {
    return;
  }

  NOTREACHED() << "SetLength() failed.";
  OnWriteComplete(cdm::FileIOClient::Status::kError);
}

void MojoCdmFileIOImpl::OnFileLengthSet(cdm::FileIOClient::Status result,
                                        base::File::Error error) {
  DVLOG(3) << __func__ << " file: " << file_name_;

  // If setting the length failed, still attempt to flush the file but make
  // sure to return an error as the state of the file is unknown.
  if (error != base::File::FILE_OK)
    result = cdm::FileIOClient::Status::kError;

  if (file_.Flush(base::Bind(&MojoCdmFileIOImpl::OnFileFlushed,
                             weak_factory_.GetWeakPtr(), result))) {
    return;
  }

  NOTREACHED() << "Flush() failed.";
  OnWriteComplete(cdm::FileIOClient::Status::kError);
}

void MojoCdmFileIOImpl::OnFileFlushed(cdm::FileIOClient::Status result,
                                      base::File::Error error) {
  DVLOG(3) << __func__ << " file: " << file_name_;

  if (error != base::File::FILE_OK)
    result = cdm::FileIOClient::Status::kError;

  OnWriteComplete(result);
}

void MojoCdmFileIOImpl::OnWriteComplete(cdm::FileIOClient::Status status) {
  DVLOG(3) << __func__ << " file: " << file_name_ << ", status: " << status;

  // Only reset the state if completing a write.
  if (state_manager_.GetState() == StateManager::OperationType::kWrite)
    state_manager_.SetState(StateManager::OperationType::kOpened);

  client_->OnWriteComplete(status);
}

void MojoCdmFileIOImpl::Close() {
  DVLOG(3) << __func__ << " file: " << file_name_;

  StateManager::OperationType state = state_manager_.GetState();
  state_manager_.SetState(StateManager::OperationType::kClosed);

  // If the file was successfully opened, close it.
  switch (state) {
    case StateManager::OperationType::kNone:
    case StateManager::OperationType::kOpening:
      break;
    case StateManager::OperationType::kOpened:
    case StateManager::OperationType::kRead:
    case StateManager::OperationType::kWrite:
      storage_->Close(file_name_,
                      base::Bind(&MojoCdmFileIOImpl::OnCloseComplete,
                                 weak_factory_.GetWeakPtr()));
      return;
    case StateManager::OperationType::kClosed:
      // Calling Close() twice is a problem.
      NOTREACHED();
      break;
  }

  OnCloseComplete(true);
}

void MojoCdmFileIOImpl::OnCloseComplete(bool status) {
  // No way to report Close() failing to the client, so ignore the result.
  DVLOG(3) << __func__ << (status ? " success" : " failure");
  delete this;
}

void MojoCdmFileIOImpl::PostTask(base::OnceClosure task) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, std::move(task));
}

}  // namespace media
