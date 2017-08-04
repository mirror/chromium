// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MUXERS_SEEKABLE_WEBM_WRITING_CALLBACK_H_
#define MEDIA_MUXERS_SEEKABLE_WEBM_WRITING_CALLBACK_H_

// #include <stdint.h>

// #include <deque>
// #include <memory>

// #include "base/callback.h"
// #include "base/macros.h"
// #include "base/numerics/safe_math.h"
// #include "base/strings/string_piece.h"
// #include "base/threading/thread_checker.h"
// #include "base/time/time.h"
// #include "base/timer/elapsed_timer.h"
// #include "media/base/media_export.h"
// #include "media/base/video_codecs.h"
#include "base/optional.h"
#include "media/muxers/mkv_reader_writer_adapter.h"
#include "media/muxers/webm_muxer.h"
#include "third_party/libwebm/source/mkvmuxer.hpp"
#include "third_party/libwebm/source/webm_parser/include/webm/callback.h"

namespace media {

// Receives calls from a webm::WebmParser and uses this input to produce a
// seekable WebM stream.
class SeekableWebmRemuxer : public webm::Callback {
 public:
  SeekableWebmRemuxer(MkvReaderWriter* target_buffer);
  ~SeekableWebmRemuxer() override;

  bool has_error() { return has_error_; }
  bool Finalize();
  bool CopyAndMoveCuesBeforeClusters(MkvReaderWriter* target_buffer);

  // webm::Callback implementation.
  webm::Status OnElementBegin(const webm::ElementMetadata& metadata,
                              webm::Action* action) override;
  webm::Status OnUnknownElement(const webm::ElementMetadata& metadata,
                                webm::Reader* reader,
                                std::uint64_t* bytes_remaining) override;
  webm::Status OnEbml(const webm::ElementMetadata& metadata,
                      const webm::Ebml& ebml) override;
  webm::Status OnVoid(const webm::ElementMetadata& metadata,
                      webm::Reader* reader,
                      std::uint64_t* bytes_remaining) override;
  webm::Status OnSegmentBegin(const webm::ElementMetadata& metadata,
                              webm::Action* action) override;
  webm::Status OnSeek(const webm::ElementMetadata& metadata,
                      const webm::Seek& seek) override;
  webm::Status OnInfo(const webm::ElementMetadata& metadata,
                      const webm::Info& info) override;
  webm::Status OnClusterBegin(const webm::ElementMetadata& metadata,
                              const webm::Cluster& cluster,
                              webm::Action* action) override;
  webm::Status OnSimpleBlockBegin(const webm::ElementMetadata& metadata,
                                  const webm::SimpleBlock& simple_block,
                                  webm::Action* action) override;
  webm::Status OnSimpleBlockEnd(const webm::ElementMetadata& metadata,
                                const webm::SimpleBlock& simple_block) override;
  webm::Status OnBlockGroupBegin(const webm::ElementMetadata& metadata,
                                 webm::Action* action) override;
  webm::Status OnBlockBegin(const webm::ElementMetadata& metadata,
                            const webm::Block& block,
                            webm::Action* action) override;
  webm::Status OnBlockEnd(const webm::ElementMetadata& metadata,
                          const webm::Block& block) override;
  webm::Status OnBlockGroupEnd(const webm::ElementMetadata& metadata,
                               const webm::BlockGroup& block_group) override;
  webm::Status OnFrame(const webm::FrameMetadata& metadata,
                       webm::Reader* reader,
                       std::uint64_t* bytes_remaining) override;
  webm::Status OnClusterEnd(const webm::ElementMetadata& metadata,
                            const webm::Cluster& cluster) override;
  webm::Status OnTrackEntry(const webm::ElementMetadata& metadata,
                            const webm::TrackEntry& track_entry) override;
  webm::Status OnCuePoint(const webm::ElementMetadata& metadata,
                          const webm::CuePoint& cue_point) override;
  webm::Status OnEditionEntry(const webm::ElementMetadata& metadata,
                              const webm::EditionEntry& edition_entry) override;
  webm::Status OnTag(const webm::ElementMetadata& metadata,
                     const webm::Tag& tag) override;
  webm::Status OnSegmentEnd(const webm::ElementMetadata& metadata) override;

 private:
  void EnsureReadBufferSizeIsAtLeast(uint64_t size_in_bytes);
  void AddVideoTrack(uint64_t track_number,
                     const std::string& codec_id,
                     const webm::Video& video_info);
  void AddAudioTrack(uint64_t track_number,
                     const std::string& codec_id,
                     const webm::Audio& audio_info);

  std::unique_ptr<MkvReaderWriterAdapter> target_buffer_adapter_;
  bool has_error_;
  mkvmuxer::Segment segment_;
  std::unique_ptr<uint8_t[]> read_buffer_;
  uint64_t read_buffer_size_in_bytes_;
  base::Optional<webm::SimpleBlock> block_;
  base::Optional<uint64_t> cluster_timecode_;
  int video_track_number_;

  DISALLOW_COPY_AND_ASSIGN(SeekableWebmRemuxer);
};

}  // namespace media

#endif  // MEDIA_MUXERS_SEEKABLE_WEBM_WRITING_CALLBACK_H_
