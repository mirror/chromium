// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mojo_cdm_file_io.h"

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

// mojom::CdmStorage takes a mojom::CdmFileReleaser reference to keep track of
// the file being used. This class represents a trivial version that simply
// owns the binding.
class MojoCdmFileIO::FileInUseTracker : public media::mojom::CdmFileReleaser {
 public:
  FileInUseTracker() : binding_(this) {}
  ~FileInUseTracker() override = default;

  mojom::CdmFileReleaserPtr CreateInterfacePtrAndBind() {
    mojom::CdmFileReleaserPtr file_in_use;
    binding_.Bind(mojo::MakeRequest(&file_in_use));

    // Force the interface pointer to do full initialization.
    file_in_use.get();
    return file_in_use;
  }

 private:
  mojo::Binding<media::mojom::CdmFileReleaser> binding_;
  DISALLOW_COPY_AND_ASSIGN(FileInUseTracker);
};

bool MojoCdmFileIO::StateManager::SetState(State op) {
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

MojoCdmFileIO::MojoCdmFileIO(mojom::CdmStoragePtr storage,
                             cdm::FileIOClient* client)
    : storage_(std::move(storage)),
      client_(client),
      file_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
      file_(file_task_runner_.get()),
      weak_factory_(this) {
  DCHECK(storage_);
  DVLOG(3) << __func__;
}

MojoCdmFileIO::~MojoCdmFileIO() {
  // The destructor is private. |this| can only be destructed through Close().
}

void MojoCdmFileIO::Open(const char* file_name, uint32_t file_name_size) {
  std::string name(file_name, file_name_size);
  DVLOG(3) << __func__ << " file: " << name;

  if (!state_manager_.SetState(StateManager::State::kOpening)) {
    PostTask(base::BindOnce(&MojoCdmFileIO::OnOpenComplete,
                            weak_factory_.GetWeakPtr(),
                            mojom::CdmStorage::Status::FAILURE, base::File()));
    return;
  }

  file_name_.swap(name);
  file_in_use_tracker_ = base::MakeUnique<FileInUseTracker>();
  storage_->Open(file_name_, file_in_use_tracker_->CreateInterfacePtrAndBind(),
                 base::BindOnce(&MojoCdmFileIO::OnOpenComplete,
                                weak_factory_.GetWeakPtr()));
}

void MojoCdmFileIO::OnOpenComplete(mojom::CdmStorage::Status status,
                                   base::File file) {
  DVLOG(3) << __func__ << " file: " << file_name_ << ", status: " << status;

  if (status == mojom::CdmStorage::Status::SUCCESS) {
    state_manager_.SetState(StateManager::State::kOpened);
    file_.SetFile(std::move(file));
  }

  client_->OnOpenComplete(ConvertStatus(status));
}

void MojoCdmFileIO::Read() {
  DVLOG(3) << __func__ << " file: " << file_name_;

  if (!state_manager_.SetState(StateManager::State::kReading)) {
    cdm::FileIOClient::Status status = state_manager_.IsReadOrWriteInProgress()
                                           ? cdm::FileIOClient::Status::kInUse
                                           : cdm::FileIOClient::Status::kError;
    PostTask(base::BindOnce(&MojoCdmFileIO::OnReadComplete,
                            weak_factory_.GetWeakPtr(), status, nullptr, 0));
    return;
  }

  if (file_.GetInfo(base::Bind(&MojoCdmFileIO::OnGetInfoComplete,
                               weak_factory_.GetWeakPtr()))) {
    return;
  }

  NOTREACHED() << "GetInfo() failed.";
  PostTask(base::BindOnce(&MojoCdmFileIO::OnReadComplete,
                          weak_factory_.GetWeakPtr(),
                          cdm::FileIOClient::Status::kError, nullptr, 0));
}

void MojoCdmFileIO::OnGetInfoComplete(base::File::Error error,
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

  if (file_.Read(
          0, info.size,
          base::Bind(&MojoCdmFileIO::OnFileRead, weak_factory_.GetWeakPtr()))) {
    return;
  }

  NOTREACHED() << "Read() failed.";
  OnReadComplete(cdm::FileIOClient::Status::kError, nullptr, 0);
}

void MojoCdmFileIO::OnFileRead(base::File::Error error,
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
    PostTask(base::BindOnce(&MojoCdmFileIO::OnWriteComplete,
                            weak_factory_.GetWeakPtr(), status));
    return;
  }

  if (data_size > kFileSizeLimit) {
    DLOG(WARNING) << __func__
                  << " Too much data to write. #bytes = " << data_size;
    PostTask(base::BindOnce(&MojoCdmFileIO::OnWriteComplete,
                            weak_factory_.GetWeakPtr(),
                            cdm::FileIOClient::Status::kError));
    return;
  }

  // FileProxy doesn't support Write() with 0 bytes, so simply set the length.
  if (data_size == 0) {
    if (!file_.SetLength(0, base::Bind(&MojoCdmFileIO::OnFileLengthSet,
                                       weak_factory_.GetWeakPtr(),
                                       cdm::FileIOClient::Status::kSuccess))) {
      NOTREACHED() << "SetLength() failed.";
      PostTask(base::BindOnce(&MojoCdmFileIO::OnWriteComplete,
                              weak_factory_.GetWeakPtr(),
                              cdm::FileIOClient::Status::kError));
    }
    return;
  }

  if (file_.Write(0, reinterpret_cast<const char*>(data), data_size,
                  base::Bind(&MojoCdmFileIO::OnFileWritten,
                             weak_factory_.GetWeakPtr()))) {
    return;
  }

  NOTREACHED() << "Write() failed.";
  PostTask(base::BindOnce(&MojoCdmFileIO::OnWriteComplete,
                          weak_factory_.GetWeakPtr(),
                          cdm::FileIOClient::Status::kError));
}

void MojoCdmFileIO::OnFileWritten(base::File::Error error, int bytes_written) {
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
                      base::Bind(&MojoCdmFileIO::OnFileLengthSet,
                                 weak_factory_.GetWeakPtr(), result))) {
    return;
  }

  NOTREACHED() << "SetLength() failed.";
  OnWriteComplete(cdm::FileIOClient::Status::kError);
}

void MojoCdmFileIO::OnFileLengthSet(cdm::FileIOClient::Status result,
                                    base::File::Error error) {
  DVLOG(3) << __func__ << " file: " << file_name_;

  // If setting the length failed, still attempt to flush the file but make
  // sure to return an error as the state of the file is unknown.
  if (error != base::File::FILE_OK)
    result = cdm::FileIOClient::Status::kError;

  if (file_.Flush(base::Bind(&MojoCdmFileIO::OnFileFlushed,
                             weak_factory_.GetWeakPtr(), result))) {
    return;
  }

  NOTREACHED() << "Flush() failed.";
  OnWriteComplete(cdm::FileIOClient::Status::kError);
}

void MojoCdmFileIO::OnFileFlushed(cdm::FileIOClient::Status result,
                                  base::File::Error error) {
  DVLOG(3) << __func__ << " file: " << file_name_;

  if (error != base::File::FILE_OK)
    result = cdm::FileIOClient::Status::kError;

  OnWriteComplete(result);
}

void MojoCdmFileIO::OnWriteComplete(cdm::FileIOClient::Status status) {
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

void MojoCdmFileIO::PostTask(base::OnceClosure task) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, std::move(task));
}

}  // namespace media
