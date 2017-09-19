// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_SOURCE_BUFFER_H_
#define MEDIA_BASE_SOURCE_BUFFER_H_

#include "base/memory/memory_pressure_listener.h"
#include "media/base/demuxer.h"
#include "media/base/demuxer_stream.h"
#include "media/base/media_export.h"
#include "media/base/source_buffer_parse_warnings.h"

namespace media {

class MEDIA_EXPORT SourceBuffer {
 public:
  enum Status {
    kOk,              // ID added w/o error.
    kNotSupported,    // Type specified is not supported.
    kReachedIdLimit,  // Reached ID limit. We can't handle any more IDs.
    kStatusMax = kReachedIdLimit
  };

  SourceBuffer();
  virtual ~SourceBuffer();

  virtual SourceBuffer::Status AddId(const std::string& id,
                                     const std::string& type,
                                     const std::string& codecs) = 0;
  virtual bool AppendData(const std::string& id,
                          const uint8_t* data,
                          size_t length,
                          base::TimeDelta append_window_start,
                          base::TimeDelta append_window_end,
                          base::TimeDelta* timestamp_offset) = 0;
  virtual void SetTracksWatcher(
      const std::string& id,
      const Demuxer::MediaTracksUpdatedCB& tracks_updated_cb) = 0;
  virtual void SetParseWarningCallback(
      const std::string& id,
      const media::SourceBufferParseWarningCB& parse_warning_cb) = 0;
  virtual void RemoveId(const std::string& id) = 0;
  virtual Ranges<base::TimeDelta> GetBufferedRanges(
      const std::string& id) const = 0;
  virtual base::TimeDelta GetHighestPresentationTimestamp(
      const std::string& id) const = 0;
  virtual void ResetParserState(const std::string& id,
                                base::TimeDelta append_window_start,
                                base::TimeDelta append_window_end,
                                base::TimeDelta* timestamp_offset) = 0;
  virtual void Remove(const std::string& id,
                      base::TimeDelta start,
                      base::TimeDelta end) = 0;
  virtual bool EvictCodedFrames(const std::string& id,
                                base::TimeDelta currentMediaTime,
                                size_t newDataSize) = 0;
  virtual void OnMemoryPressure(
      base::TimeDelta currentMediaTime,
      base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level,
      bool force_instant_gc) = 0;
  virtual double GetDuration() = 0;
  virtual double GetDuration_Locked() = 0;
  virtual void SetDuration(double duration) = 0;
  virtual bool IsParsingMediaSegment(const std::string& id) = 0;
  virtual void SetSequenceMode(const std::string& id, bool sequence_mode) = 0;
  virtual void SetGroupStartTimestampIfInSequenceMode(
      const std::string& id,
      base::TimeDelta timestamp_offset) = 0;

  virtual void MarkEndOfStream(PipelineStatus status) = 0;
  virtual void UnmarkEndOfStream() = 0;
  virtual void Shutdown() = 0;
  virtual void SetMemoryLimitsForTest(DemuxerStream::Type type,
                                      size_t memory_limit) = 0;
  virtual Ranges<base::TimeDelta> GetBufferedRanges() const = 0;
};

}  // namespace media

#endif  // MEDIA_BASE_SOURCE_BUFFER_H_
