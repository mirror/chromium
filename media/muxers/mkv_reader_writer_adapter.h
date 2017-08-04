// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MUXERS_MKV_READER_WRITER_ADAPTER_H_
#define MEDIA_MUXERS_MKV_READER_WRITER_ADAPTER_H_

#include <memory>

#include "media/muxers/mkv_reader_writer.h"
#include "media/muxers/webm_muxer.h"
#include "third_party/libwebm/source/mkvmuxer.hpp"
#include "third_party/libwebm/source/mkvparser/mkvparser.h"

namespace media {

// Adapter that delegates mkvmuxer::IMkvWriter and mkvparser::IMkvReader to a
// given MkvReaderWriter instance.
class MkvReaderWriterAdapter : public mkvmuxer::IMkvWriter,
                               public mkvparser::IMkvReader {
 public:
  explicit MkvReaderWriterAdapter(
      std::unique_ptr<MkvReaderWriter> reader_writer);
  explicit MkvReaderWriterAdapter(
      MkvReaderWriter* reader_writer);
  ~MkvReaderWriterAdapter() override;

  int Read(long long pos, long len, unsigned char* buf) override;
  int Length(long long* total, long long* available) override;
  mkvmuxer::int32 Write(const void* buf, mkvmuxer::uint32 len) override;
  mkvmuxer::int64 Position() const override;
  mkvmuxer::int32 Position(mkvmuxer::int64 position) override;
  bool Seekable() const override;
  void ElementStartNotify(mkvmuxer::uint64 element_id,
                          mkvmuxer::int64 position) override;

 private:
  std::unique_ptr<MkvReaderWriter> optional_ownership_;
  MkvReaderWriter* reader_writer_;
};

}  // namespace media

#endif  // MEDIA_MUXERS_MKV_READER_WRITER_ADAPTER_H_
