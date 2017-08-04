// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media_recorder/memory_buffered_mkv_reader_writer.h"

#include <algorithm>

namespace content {

MemoryBufferedMkvReaderWriter::MemoryBufferedMkvReaderWriter(
    int64_t read_buffer_size_in_bytes,
    int64_t write_buffer_size_in_bytes,
    std::unique_ptr<media::MkvReaderWriter> target)
    : target_(std::move(target)),
      read_buffer_(new uint8_t[read_buffer_size_in_bytes]),
      read_buffer_size_in_bytes_(read_buffer_size_in_bytes),
      read_buffer_start_pos_(0),
      num_bytes_in_read_buffer_(0),
      write_buffer_(new uint8_t[write_buffer_size_in_bytes]),
      write_buffer_size_in_bytes_(write_buffer_size_in_bytes),
      num_bytes_in_write_buffer_(0) {}

MemoryBufferedMkvReaderWriter::~MemoryBufferedMkvReaderWriter() = default;

int MemoryBufferedMkvReaderWriter::Read(long long pos,
                                        long len,
                                        unsigned char* buf) {
  // LOG(ERROR) << "Requesting read from " << pos << " of size " << len;
  if (PushWriteBufferToTarget() != kSuccessResultCode) {
    return kFailureResultCode;
  }
  // LOG(ERROR) << "read_buffer_start_pos_ = " << read_buffer_start_pos_;
  // LOG(ERROR) << "num_bytes_in_read_buffer_ = " << num_bytes_in_read_buffer_;
  if (pos >= read_buffer_start_pos_ &&
      (pos + len) <= (read_buffer_start_pos_ + num_bytes_in_read_buffer_)) {
    // LOG(ERROR) << "Reading from memory";
    ReadFromMemoryBuffer(pos, len, buf);
    return kSuccessResultCode;
  }
  if (len > read_buffer_size_in_bytes_) {
    // Read directly from target.
    return target_->Read(pos, len, buf);
  }
    // LOG(ERROR) << "Buffering from file";
  if (FillMemoryReadBuffer(pos) != kSuccessResultCode) {
    // LOG(ERROR) << "1";
    return kFailureResultCode;
  }
  ReadFromMemoryBuffer(pos, len, buf);
  return kSuccessResultCode;
}

int MemoryBufferedMkvReaderWriter::Length(long long* total,
                                          long long* available) {
  if (target_->Length(total, available) != kSuccessResultCode) {
    return kFailureResultCode;
  }
  *total += num_bytes_in_write_buffer_;
  *available += num_bytes_in_write_buffer_;
  return kSuccessResultCode;
}

int32_t MemoryBufferedMkvReaderWriter::Write(const void* buf, uint32_t len) {
  if (num_bytes_in_write_buffer_ + len <= write_buffer_size_in_bytes_) {
    WriteToMemoryBuffer(buf, len);
    return kSuccessResultCode;
  }
  if (PushWriteBufferToTarget() != kSuccessResultCode) {
    return kFailureResultCode;
  }
  if (len < write_buffer_size_in_bytes_) {
    WriteToMemoryBuffer(buf, len);
    return kSuccessResultCode;
  }
  WriteDirectlyToTarget(buf, len);
  return kSuccessResultCode;
}

int64_t MemoryBufferedMkvReaderWriter::Position() const {
  return target_->Position() + num_bytes_in_write_buffer_;
}

int32_t MemoryBufferedMkvReaderWriter::Position(int64_t position) {
  if (PushWriteBufferToTarget() != kSuccessResultCode) {
    return kFailureResultCode;
  }
  return target_->Position(position);
}

bool MemoryBufferedMkvReaderWriter::Seekable() const {
  return true;
}

void MemoryBufferedMkvReaderWriter::ElementStartNotify(uint64_t element_id,
                                                       int64_t position) {}

void MemoryBufferedMkvReaderWriter::ReadFromMemoryBuffer(long long pos,
                                                         long len,
                                                         unsigned char* buf) {
  int64_t offset_in_read_buffer = pos - read_buffer_start_pos_;
  memcpy(buf, read_buffer_.get() + offset_in_read_buffer, len);
}

int32_t MemoryBufferedMkvReaderWriter::FillMemoryReadBuffer(long long pos) {
  long long total_bytes = 0;
  long long available_bytes = 0;
  if (Length(&total_bytes, &available_bytes) != kSuccessResultCode) {
    // LOG(ERROR) << "2";
    return kFailureResultCode;
  }
  if (available_bytes < pos) {
   return kFailureResultCode;
  }
  int64_t num_bytes_to_read_from_target = std::min(
      static_cast<int64_t>(available_bytes - pos), read_buffer_size_in_bytes_);
  if (num_bytes_to_read_from_target == 0) {
    return kSuccessResultCode;
  }
  read_buffer_start_pos_ = pos;
  num_bytes_in_read_buffer_ = num_bytes_to_read_from_target;
  // LOG(ERROR) << "available_bytes = " << available_bytes;
  // LOG(ERROR) << "Trying to read " << num_bytes_in_read_buffer_ << " bytes";
  auto result_code = target_->Read(pos, num_bytes_to_read_from_target, read_buffer_.get());
  // LOG(ERROR) << "result = " << result_code;
  return result_code;
}

void MemoryBufferedMkvReaderWriter::WriteToMemoryBuffer(const void* buf,
                                                        uint32_t len) {
  memcpy(write_buffer_.get() + num_bytes_in_write_buffer_, buf, len);
  num_bytes_in_write_buffer_ += len;
}

int32_t MemoryBufferedMkvReaderWriter::WriteDirectlyToTarget(const void* buf,
                                                             uint32_t len) {
  return target_->Write(buf, len);
}

int32_t MemoryBufferedMkvReaderWriter::PushWriteBufferToTarget() {
  if (num_bytes_in_write_buffer_ == 0)
    return kSuccessResultCode;
  auto num_bytes_to_push = num_bytes_in_write_buffer_;
  num_bytes_in_write_buffer_ = 0;
  return target_->Write(write_buffer_.get(), num_bytes_to_push);
}

}  // namespace content
