// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MUXERS_MKV_READER_WRITER_H_
#define MEDIA_MUXERS_MKV_READER_WRITER_H_

#include <cstdint>

// #include <deque>
#include <memory>

// #include "base/callback.h"
#include "base/macros.h"
// #include "base/numerics/safe_math.h"
// #include "base/strings/string_piece.h"
// #include "base/threading/thread_checker.h"
// #include "base/time/time.h"
// #include "base/timer/elapsed_timer.h"
#include "media/base/audio_parameters.h"
#include "media/base/media_export.h"
// #include "media/base/video_codecs.h"
// #include "third_party/libwebm/source/mkvmuxer.hpp"
// #include "third_party/libwebm/source/mkvparser/mkvparser.h"
// #include "ui/gfx/geometry/size.h"

namespace media {

// Pure interfaces for clients to pass instances into
// ConvertLiveModeRecordingToFileModeRecording. The exposed methods match the
// ones from both mkvmuxer::IMkvWriter and mkvparser::IMkvReader, which we cannot
// apply MEDIA_EXPORT to directly.
class MEDIA_EXPORT MkvReader {
 public:
  virtual ~MkvReader() {}
  // Methods matching mkvparser::IMkvReader
  // Based on usage in libwebm, a return value 0 appears to indicate
  // success and negative values indicate failure.
  virtual int Read(long long pos, long len, unsigned char* buf) = 0;
  virtual int Length(long long* total, long long* available) = 0;
};

class MEDIA_EXPORT MkvReaderWriter : public MkvReader {
 public:
  // Methods matching mkvmuxer::IMkvWriter
  // For the methods returning int32_t, based on usage in libwebm, a
  // return value 0 appears to indicate success and negative values indicate
  // failure.
  virtual int32_t Write(const void* buf, uint32_t len) = 0;
  virtual int64_t Position() const = 0;
  virtual int32_t Position(int64_t position) = 0;
  virtual bool Seekable() const = 0;
  virtual void ElementStartNotify(uint64_t element_id, int64_t position) = 0;
};

}  // namespace media

#endif  // MEDIA_MUXERS_MKV_READER_WRITER_H_
