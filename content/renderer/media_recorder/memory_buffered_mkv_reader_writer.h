// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_RECORDER_MEMORY_BUFFERED_MKV_READER_WRITER_H_
#define CONTENT_RENDERER_MEDIA_RECORDER_MEMORY_BUFFERED_MKV_READER_WRITER_H_

#include <memory>

#include "media/muxers/mkv_reader_writer.h"
#include "services/file/public/interfaces/restricted_file_system.mojom.h"

namespace content {

// Decorator for media::MkvReaderWriter that accumulates small writes in memory
// before sending them as larger chunks to the given |target|.
class MemoryBufferedMkvReaderWriter : public media::MkvReaderWriter {
 public:
  static constexpr int kSuccessResultCode = 0;
  static constexpr int kFailureResultCode = -1;

  MemoryBufferedMkvReaderWriter(int64_t read_buffer_size_in_bytes,
                                int64_t write_buffer_size_in_bytes,
                                std::unique_ptr<media::MkvReaderWriter> target);
  ~MemoryBufferedMkvReaderWriter() override;

  // IMkvReader
  int Read(long long pos, long len, unsigned char* buf) override;
  int Length(long long* total, long long* available) override;

  // IMkvWriter
  int32_t Write(const void* buf, uint32_t len) override;
  int64_t Position() const override;
  int32_t Position(int64_t position) override;
  bool Seekable() const override;
  void ElementStartNotify(uint64_t element_id, int64_t position) override;

 private:
  void ReadFromMemoryBuffer(long long pos, long len, unsigned char* buf);
  int32_t FillMemoryReadBuffer(long long pos);
  void WriteToMemoryBuffer(const void* buf, uint32_t len);
  int32_t WriteDirectlyToTarget(const void* buf, uint32_t len);
  int32_t PushWriteBufferToTarget();

  std::unique_ptr<media::MkvReaderWriter> target_;

  std::unique_ptr<uint8_t[]> read_buffer_;
  int64_t read_buffer_size_in_bytes_;
  int64_t read_buffer_start_pos_;
  int64_t num_bytes_in_read_buffer_;

  std::unique_ptr<uint8_t[]> write_buffer_;
  int64_t write_buffer_size_in_bytes_;
  int64_t num_bytes_in_write_buffer_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_RECORDER_MEMORY_BUFFERED_MKV_READER_WRITER_H_
