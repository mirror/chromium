// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_FILTERS_SOURCE_BUFFER_STREAM_H_
#define MEDIA_FILTERS_SOURCE_BUFFER_STREAM_H_

#include <deque>
#include <list>
#include <utility>

#include "base/memory/ref_counted.h"
#include "media/base/audio_decoder_config.h"
#include "media/base/media_export.h"
#include "media/base/ranges.h"
#include "media/base/stream_parser_buffer.h"
#include "media/base/video_decoder_config.h"

namespace media {

class SourceBufferRange;

// SourceBufferStream is a data structure that stores media Buffers in ranges.
// Buffers can be appended out of presentation order. Buffers are retrieved by
// seeking to the desired start point and calling GetNextBuffer(). Buffers are
// returned in sequential presentation order.
class MEDIA_EXPORT SourceBufferStream {
 public:
  typedef std::deque<scoped_refptr<StreamParserBuffer> > BufferQueue;

  SourceBufferStream();
  explicit SourceBufferStream(const AudioDecoderConfig& audio_config);
  explicit SourceBufferStream(const VideoDecoderConfig& video_config);

  ~SourceBufferStream();

  // Add the |buffers| to the SourceBufferStream. Buffers within the queue are
  // expected to be in order, but multiple calls to Append() may add buffers out
  // of order or overlapping. Assumes all buffers within |buffers| are in
  // presentation order and are non-overlapping.
  // |media_segment_start_time| refers to the starting timestamp for the media
  // segment to which these buffers belong.
  // Returns true if Append() was successful, false if |buffers| are not added.
  // TODO(vrk): Implement garbage collection. (crbug.com/125070)
  bool Append(const BufferQueue& buffers,
              base::TimeDelta media_segment_start_time);

  // Changes the SourceBufferStream's state so that it will start returning
  // buffers starting from the closest keyframe before |timestamp|.
  void Seek(base::TimeDelta timestamp);

  // Returns true if the SourceBufferStream has seeked to a time without
  // buffered data and is waiting for more data to be appended.
  bool IsSeekPending() const;

  // Fills |out_buffer| with a new buffer. Buffers are presented in order from
  // the last call to Seek(), or starting with the first buffer appended if
  // Seek() has not been called yet.
  // |out_buffer|'s timestamp may be earlier than the |timestamp| passed to
  // the last Seek() call.
  // Returns true if |out_buffer| is filled with a valid buffer, false if
  // there is not enough data buffered to fulfill the request.
  bool GetNextBuffer(scoped_refptr<StreamParserBuffer>* out_buffer);

  // Returns a list of the buffered time ranges.
  Ranges<base::TimeDelta> GetBufferedTime() const;

  // Notifies this SourceBufferStream that EndOfStream has been called and that
  // GetNextBuffer() should return EOS buffers after all other buffered data.
  // Returns false if called when there is a gap between the current position
  // and the end of the buffered data.
  void EndOfStream();

  // Returns true if this SourceBufferStream can successfully call EndOfStream()
  // (if there are no gaps between the current position and the remaining data).
  bool CanEndOfStream() const;

  const AudioDecoderConfig& GetCurrentAudioDecoderConfig() {
    return audio_config_;
  }
  const VideoDecoderConfig& GetCurrentVideoDecoderConfig() {
    return video_config_;
  }

 private:
  typedef std::list<SourceBufferRange*> RangeList;

  // Appends |new_buffers| into |range_for_new_buffers_itr|, handling start and
  // end overlaps if necessary.
  void InsertIntoExistingRange(
      const RangeList::iterator& range_for_new_buffers_itr,
      const BufferQueue& new_buffers);

  // Resolve overlapping ranges such that no ranges overlap anymore.
  // |range_with_new_buffers_itr| points to the range that has newly appended
  // buffers.
  void ResolveCompleteOverlaps(
      const RangeList::iterator& range_with_new_buffers_itr);
  void ResolveEndOverlap(const RangeList::iterator& range_with_new_buffers_itr);

  // This method is a bit tricky to describe. When what would have been the
  // next buffer returned from |selected_range_| is overlapped by new data,
  // the |selected_range_| seeks forward to the next keyframe after (or at) the
  // next buffer timestamp and the overlapped buffers are deleted. But for
  // smooth playback between the old data to the new data's keyframe, some of
  // these |deleted_buffers| may be temporarily saved into |track_buffer_|.
  // UpdateTrackBuffer() takes these |deleted_buffers| and decides whether it
  // wants to save any buffers into |track_buffer_|.
  // TODO(vrk): This is a little crazy! Ideas for cleanup in crbug.com/129623.
  void UpdateTrackBuffer(const BufferQueue& deleted_buffers);

  // Checks to see if |range_with_new_buffers_itr| can be merged with the range
  // next to it, and merges them if so.
  void MergeWithAdjacentRangeIfNecessary(
      const RangeList::iterator& range_with_new_buffers_itr);

  // Helper method that returns the timestamp for the next buffer that
  // |selected_range_| will return from GetNextBuffer() call, or kNoTimestamp()
  // if in between seeking (i.e. |selected_range_| is null).
  base::TimeDelta GetNextBufferTimestamp();

  // Finds the range into which |new_buffers| should be inserted and returns the
  // iterator pointing to it. Returns |ranges_.end()| if no existing range
  // should contain |new_buffers|.
  RangeList::iterator FindExistingRangeFor(const BufferQueue& new_buffers);

  // Inserts |new_range| into |ranges_| preserving sorted order. Returns an
  // iterator in |ranges_| that points to |new_range|.
  RangeList::iterator AddToRanges(SourceBufferRange* new_range);

  // Returns an iterator that points to the place in |ranges_| where
  // |selected_range_| lives.
  RangeList::iterator GetSelectedRangeItr();

  // List of disjoint buffered ranges, ordered by start time.
  RangeList ranges_;

  AudioDecoderConfig audio_config_;
  VideoDecoderConfig video_config_;

  // True if more data needs to be appended before the Seek() can complete,
  // false if no Seek() has been requested or the Seek() is completed.
  bool seek_pending_;

  // Timestamp of the last request to Seek().
  base::TimeDelta seek_buffer_timestamp_;

  // Pointer to the seeked-to Range. This is the range from which
  // GetNextBuffer() calls are fulfilled after the |track_buffer_| has been
  // emptied.
  SourceBufferRange* selected_range_;

  // Queue of the next buffers to be returned from calls to GetNextBuffer(). If
  // |track_buffer_| is empty, return buffers from |selected_range_|.
  BufferQueue track_buffer_;

  // True when EndOfStream() has been called and GetNextBuffer() should return
  // EOS buffers for read requests beyond the buffered data.  False initially.
  bool end_of_stream_;

  DISALLOW_COPY_AND_ASSIGN(SourceBufferStream);
};

}  // namespace media

#endif  // MEDIA_FILTERS_SOURCE_BUFFER_STREAM_H_
