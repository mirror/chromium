// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media_recorder/file_service_mkv_reader_writer.h"

namespace content {

FileServiceMkvReaderWriter::FileServiceMkvReaderWriter(
    filesystem::mojom::FilePtr file_ptr)
    : file_ptr_(std::move(file_ptr)), write_pos_(0) {}

FileServiceMkvReaderWriter::~FileServiceMkvReaderWriter() = default;

int FileServiceMkvReaderWriter::Read(long long pos,
                                     long len,
                                     unsigned char* buf) {
  if (len <= 0)
    return kSuccessResultCode;
  base::Optional<std::vector<uint8_t>> received_data;
  filesystem::mojom::FileError result_code;
  bool success = file_ptr_->Read(
      base::saturated_cast<uint32_t>(len), base::saturated_cast<int64_t>(pos),
      filesystem::mojom::Whence::FROM_BEGIN, &result_code, &received_data);
  if (!success) {
    // LOG(ERROR) << "Failed to read " << len << " bytes from pos " << pos;
    return kFailureResultCode;
  }
  if (result_code != filesystem::mojom::FileError::OK) {
    // LOG(ERROR) << "Failed to read " << len << " bytes from pos " << pos;
    return kFailureResultCode;
  }
  if (!received_data.has_value()) {
    // LOG(ERROR) << "Failed to read " << len << " bytes from pos " << pos;
    return kFailureResultCode;
  }
  if (received_data.value().size() != base::saturated_cast<size_t>(len)) {
    // LOG(ERROR) << "Failed to read " << len << " bytes from pos " << pos;
    return kFailureResultCode;
  }
  memcpy(buf, &received_data.value()[0], received_data.value().size());
  // LOG(ERROR) << "Successfully read " << len << " bytes from pos " << pos;
  return kSuccessResultCode;
}

int FileServiceMkvReaderWriter::Length(long long* total, long long* available) {
  filesystem::mojom::FileError result_code;
  filesystem::mojom::FileInformationPtr received_info_ptr;
  bool success = file_ptr_->Stat(&result_code, &received_info_ptr);
  if (!success) {
    // LOG(ERROR) << "Failed get file info";
    return kFailureResultCode;
  }
  if (result_code != filesystem::mojom::FileError::OK) {
    // LOG(ERROR) << "Failed get file info";
    return kFailureResultCode;
  }
  *total = base::saturated_cast<long long>(received_info_ptr->size);
  *available = *total;
  // LOG(ERROR) << "Successfully obtained file info. Total = " << *total;
  return kSuccessResultCode;
}

int32_t FileServiceMkvReaderWriter::Write(const void* buf, uint32_t len) {
  filesystem::mojom::FileError result_code;
  uint32_t num_bytes_written = 0;
  // Wrap |buf| in a std::vector<>
  std::vector<uint8_t> bytes_to_write(len);
  memcpy(&bytes_to_write[0], buf, len);
  bool success = file_ptr_->Write(bytes_to_write, write_pos_,
                                  filesystem::mojom::Whence::FROM_BEGIN,
                                  &result_code, &num_bytes_written);
  if (!success) {
    // LOG(ERROR) << "Failed to write " << len << " bytes from pos "
    //            << write_pos_;
    return kFailureResultCode;
  }
  if (result_code != filesystem::mojom::FileError::OK) {
    // LOG(ERROR) << "Failed to write " << len << " bytes from pos "
    //            << write_pos_;
    return kFailureResultCode;
  }
  if (num_bytes_written < len) {
    // LOG(ERROR) << "Failed to write " << len << " bytes from pos "
    //            << write_pos_;
    return kFailureResultCode;
  }
  // LOG(ERROR) << "Successfully wrote " << len << " bytes from pos "
  //            << write_pos_;
  write_pos_ += len;
  return kSuccessResultCode;
}

int64_t FileServiceMkvReaderWriter::Position() const {
  return static_cast<int64_t>(write_pos_);
}

int32_t FileServiceMkvReaderWriter::Position(int64_t position) {
  write_pos_ = position;
  // LOG(ERROR) << "Set write pos to " << write_pos_;
  return kSuccessResultCode;
}

bool FileServiceMkvReaderWriter::Seekable() const {
  return true;
}
void FileServiceMkvReaderWriter::ElementStartNotify(uint64_t element_id,
                                                    int64_t position) {}

}  // namespace content
