// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_RECORDER_FILE_SERVICE_MKV_READER_WRITER_H_
#define CONTENT_RENDERER_MEDIA_RECORDER_FILE_SERVICE_MKV_READER_WRITER_H_

#include <memory>

#include "media/muxers/mkv_reader_writer.h"
#include "services/file/public/interfaces/restricted_file_system.mojom.h"

namespace content {

class FileServiceMkvReaderWriter : public media::MkvReaderWriter {
 public:
  static constexpr int kSuccessResultCode = 0;
  static constexpr int kFailureResultCode = -1;

  FileServiceMkvReaderWriter(filesystem::mojom::FilePtr file_ptr);
  ~FileServiceMkvReaderWriter() override;

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
  filesystem::mojom::FilePtr file_ptr_;
  int64_t write_pos_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_RECORDER_FILE_SERVICE_MKV_READER_WRITER_H_
