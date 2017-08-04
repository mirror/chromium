// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/muxers/seekable_webm_remuxer.h"

#include "base/memory/ptr_util.h"
#include "media/filters/opus_constants.h"
#include "media/muxers/webm_muxer_utils.h"

namespace {
 static const int64_t kNanosecondsPerMillisecond = 1000000;
}

namespace media {

SeekableWebmRemuxer::SeekableWebmRemuxer(MkvReaderWriter* target_buffer)
    : has_error_(false),
      read_buffer_size_in_bytes_(0u),
      video_track_number_(-1) {
  target_buffer_adapter_ =
      base::MakeUnique<MkvReaderWriterAdapter>(target_buffer);
  segment_.Init(target_buffer_adapter_.get());
  segment_.set_mode(mkvmuxer::Segment::kFile);
  segment_.OutputCues(true);
  mkvmuxer::SegmentInfo* const info = segment_.GetSegmentInfo();
  info->set_writing_app("Chrome");
  info->set_muxing_app("Chrome");
}

SeekableWebmRemuxer::~SeekableWebmRemuxer() {}

bool SeekableWebmRemuxer::Finalize() {
  LOG(ERROR) << "Finalizing segment";
  return segment_.Finalize();
}

bool SeekableWebmRemuxer::CopyAndMoveCuesBeforeClusters(
    MkvReaderWriter* target_buffer) {
  MkvReaderWriterAdapter target_buffer_adapter(target_buffer);
  LOG(ERROR) << "CopyAndMoveCuesBeforeClusters";
  return segment_.CopyAndMoveCuesBeforeClusters(target_buffer_adapter_.get(),
                                                &target_buffer_adapter);
}

webm::Status SeekableWebmRemuxer::OnElementBegin(
    const webm::ElementMetadata& metadata,
    webm::Action* action) {
  // LOG(ERROR) << __FUNCTION__;
  return webm::Callback::OnElementBegin(metadata, action);
}

webm::Status SeekableWebmRemuxer::OnUnknownElement(
    const webm::ElementMetadata& metadata,
    webm::Reader* reader,
    std::uint64_t* bytes_remaining) {
  // LOG(ERROR) << __FUNCTION__;
  return webm::Callback::OnUnknownElement(metadata, reader, bytes_remaining);
}

webm::Status SeekableWebmRemuxer::OnEbml(const webm::ElementMetadata& metadata,
                                         const webm::Ebml& ebml) {
  // LOG(ERROR) << __FUNCTION__;
  return webm::Callback::OnEbml(metadata, ebml);
}

webm::Status SeekableWebmRemuxer::OnVoid(const webm::ElementMetadata& metadata,
                                         webm::Reader* reader,
                                         std::uint64_t* bytes_remaining) {
  // LOG(ERROR) << __FUNCTION__;
  return webm::Callback::OnVoid(metadata, reader, bytes_remaining);
}

webm::Status SeekableWebmRemuxer::OnSegmentBegin(
    const webm::ElementMetadata& metadata,
    webm::Action* action) {
  // LOG(ERROR) << __FUNCTION__;
  return webm::Callback::OnSegmentBegin(metadata, action);
}

webm::Status SeekableWebmRemuxer::OnSeek(const webm::ElementMetadata& metadata,
                                         const webm::Seek& seek) {
  // LOG(ERROR) << __FUNCTION__;
  return webm::Callback::OnSeek(metadata, seek);
}

webm::Status SeekableWebmRemuxer::OnInfo(const webm::ElementMetadata& metadata,
                                         const webm::Info& info) {
  // LOG(ERROR) << __FUNCTION__;
  // Verify that timecodes are in milliseconds.
  DCHECK_EQ(1000000ull, info.timecode_scale.value());
  return webm::Callback::OnInfo(metadata, info);
}

webm::Status SeekableWebmRemuxer::OnClusterBegin(
    const webm::ElementMetadata& metadata,
    const webm::Cluster& cluster,
    webm::Action* action) {
  // LOG(ERROR) << __FUNCTION__;
  DCHECK(cluster.timecode.is_present());
  cluster_timecode_ = cluster.timecode.value();
  // LOG(ERROR) << "cluster_timecode_ = " << cluster_timecode_.value();
  return webm::Callback::OnClusterBegin(metadata, cluster, action);
}

webm::Status SeekableWebmRemuxer::OnSimpleBlockBegin(
    const webm::ElementMetadata& metadata,
    const webm::SimpleBlock& simple_block,
    webm::Action* action) {
  // LOG(ERROR) << __FUNCTION__;
  DCHECK(!block_.has_value());
  block_ = simple_block;
  return webm::Callback::OnSimpleBlockBegin(metadata, simple_block, action);
}

webm::Status SeekableWebmRemuxer::OnSimpleBlockEnd(
    const webm::ElementMetadata& metadata,
    const webm::SimpleBlock& simple_block) {
  // LOG(ERROR) << __FUNCTION__;
  DCHECK(block_.has_value());
  block_.reset();
  return webm::Callback::OnSimpleBlockEnd(metadata, simple_block);
}

webm::Status SeekableWebmRemuxer::OnBlockGroupBegin(
    const webm::ElementMetadata& metadata,
    webm::Action* action) {
  // LOG(ERROR) << __FUNCTION__;
  return webm::Callback::OnBlockGroupBegin(metadata, action);
}

webm::Status SeekableWebmRemuxer::OnBlockBegin(
    const webm::ElementMetadata& metadata,
    const webm::Block& block,
    webm::Action* action) {
  // LOG(ERROR) << __FUNCTION__;
  return webm::Callback::OnBlockBegin(metadata, block, action);
}

webm::Status SeekableWebmRemuxer::OnBlockEnd(
    const webm::ElementMetadata& metadata,
    const webm::Block& block) {
  // LOG(ERROR) << __FUNCTION__;
  return webm::Callback::OnBlockEnd(metadata, block);
}

webm::Status SeekableWebmRemuxer::OnBlockGroupEnd(
    const webm::ElementMetadata& metadata,
    const webm::BlockGroup& block_group) {
  // LOG(ERROR) << __FUNCTION__;
  return webm::Callback::OnBlockGroupEnd(metadata, block_group);
}

webm::Status SeekableWebmRemuxer::OnFrame(const webm::FrameMetadata& metadata,
                                          webm::Reader* reader,
                                          std::uint64_t* bytes_remaining) {
  // LOG(ERROR) << __FUNCTION__;
  // Q: How can we find out if frame has additional data (i.e. alpha) attached?
  //  O: We can test this using a fake camera with alpha.
  // DCHECK(encoded_data->data());
  // if (!encoded_alpha || encoded_alpha->empty()) {
  //   }
  //   return true;
  // }

  uint64_t frame_size_in_bytes = *bytes_remaining;
  EnsureReadBufferSizeIsAtLeast(frame_size_in_bytes);
  webm::Status status;
  uint64_t offset_in_read_buffer = 0;
  do {
    uint64_t num_actually_read = 0;
    status = reader->Read(*bytes_remaining,
                          read_buffer_.get() + offset_in_read_buffer,
                          &num_actually_read);
    *bytes_remaining -= num_actually_read;
    offset_in_read_buffer += num_actually_read;
  } while (status.code == webm::Status::kOkPartial);

  DCHECK(cluster_timecode_.has_value());
  DCHECK(block_.has_value());
  const uint64_t timecode_in_ms =
      cluster_timecode_.value() + block_.value().timecode;
  const uint64_t timecode_in_ns = timecode_in_ms * kNanosecondsPerMillisecond;
  // LOG(ERROR) << "Frame track_number = " << block_.value().track_number;
  // LOG(ERROR) << "Frame timecode = " << timecode;
  // LOG(ERROR) << "frame_size_in_bytes = " << frame_size_in_bytes;

  if (!segment_.AddFrame(read_buffer_.get(), frame_size_in_bytes,
                         block_.value().track_number, timecode_in_ns,
                         block_.value().is_key_frame)) {
    LOG(ERROR) << "Error during AddFrame";
    has_error_ = true;
  }

  if (static_cast<int>(block_.value().track_number) == video_track_number_ &&
      block_.value().is_key_frame) {
    LOG(ERROR) << "Adding cue point for video key frame";
    segment_.AddCuePoint(timecode_in_ns, block_.value().track_number);
  }

  // DCHECK(encoded_alpha->data());
  // if (!segment_.AddFrameWithAdditional(
  //         read_buffer_.get(),
  //         frame_size_in_bytes,
  //         reinterpret_cast<const uint8_t*>(encoded_alpha->data()),
  //         encoded_alpha->size(), 1 /* add_id */, track_index,
  //         timestamp_in_ns, is_key_frame)) {
  //   has_error_ = true;
  // }

  return webm::Callback::OnFrame(metadata, reader, bytes_remaining);
}

void SeekableWebmRemuxer::EnsureReadBufferSizeIsAtLeast(
    uint64_t size_in_bytes) {
  if (read_buffer_size_in_bytes_ < size_in_bytes) {
    read_buffer_ = std::unique_ptr<uint8_t[]>(new uint8_t[size_in_bytes]);
    read_buffer_size_in_bytes_ = size_in_bytes;
  }
}

webm::Status SeekableWebmRemuxer::OnClusterEnd(
    const webm::ElementMetadata& metadata,
    const webm::Cluster& cluster) {
  // LOG(ERROR) << __FUNCTION__;
  cluster_timecode_.reset();
  return webm::Callback::OnClusterEnd(metadata, cluster);
}

webm::Status SeekableWebmRemuxer::OnTrackEntry(
    const webm::ElementMetadata& metadata,
    const webm::TrackEntry& track_entry) {
  // LOG(ERROR) << __FUNCTION__;
  switch (track_entry.track_type.value()) {
    case webm::TrackType::kVideo:
      LOG(ERROR) << "Entering Video Track";
      DCHECK(track_entry.track_number.is_present());
      DCHECK(track_entry.codec_id.is_present());
      DCHECK(track_entry.video.is_present());
      AddVideoTrack(track_entry.track_number.value(),
                    track_entry.codec_id.value(), track_entry.video.value());
      break;
    case webm::TrackType::kAudio:
      LOG(ERROR) << "Entering Audio Track";
      AddAudioTrack(track_entry.track_number.value(),
                    track_entry.codec_id.value(), track_entry.audio.value());
      break;
    case webm::TrackType::kComplex:
    case webm::TrackType::kLogo:
    case webm::TrackType::kSubtitle:
    case webm::TrackType::kButtons:
    case webm::TrackType::kControl:
      LOG(ERROR) << "Skipping unknown track type";
      break;
  }
  return webm::Callback::OnTrackEntry(metadata, track_entry);
}

void SeekableWebmRemuxer::AddVideoTrack(uint64_t track_number,
                                        const std::string& codec_id,
                                        const webm::Video& video_info) {
  DCHECK(video_info.pixel_width.is_present());
  DCHECK(video_info.pixel_height.is_present());
  video_track_number_ = track_number;
  LOG(ERROR) << "video_track_number_ = " << video_track_number_;
  if (segment_.AddVideoTrack(video_info.pixel_width.value(),
                             video_info.pixel_height.value(),
                             track_number) != track_number) {
    LOG(ERROR) << "Error during AddVideoTrack";
    has_error_ = true;
    return;
  }
  mkvmuxer::VideoTrack* const video_track =
      reinterpret_cast<mkvmuxer::VideoTrack*>(
          segment_.GetTrackByNumber(track_number));
  DCHECK(video_track);
  LOG(ERROR) << "codec_id = " << codec_id;
  video_track->set_codec_id(codec_id.c_str());
  if (video_info.alpha_mode.is_present()) {
    video_track->SetAlphaMode(video_info.alpha_mode.value());
    if (video_info.alpha_mode.value() == mkvmuxer::VideoTrack::kAlpha) {
      LOG(ERROR) << "Using AlphaMode";
      // Alpha channel, if present, is stored in a BlockAdditional next to the
      // associated opaque Block, see
      // https://matroska.org/technical/specs/index.html#BlockAdditional.
      // This follows Method 1 for VP8 encoding of A-channel described on
      // http://wiki.webmproject.org/alpha-channel.
      video_track->set_max_block_additional_id(1);
    }
  }
}

void SeekableWebmRemuxer::AddAudioTrack(uint64_t track_number,
                                        const std::string& codec_id,
                                        const webm::Audio& audio_info) {
  DCHECK(audio_info.sampling_frequency.is_present());
  DCHECK(audio_info.channels.is_present());
  if (segment_.AddAudioTrack(audio_info.sampling_frequency.value(),
                             audio_info.channels.value(),
                             track_number) != track_number) {
    LOG(ERROR) << "Error during AddAudioTrack";
    has_error_ = true;
    return;
  }
  mkvmuxer::AudioTrack* const audio_track =
      reinterpret_cast<mkvmuxer::AudioTrack*>(
          segment_.GetTrackByNumber(track_number));
  DCHECK(audio_track);
  LOG(ERROR) << "codec_id = " << codec_id;
  audio_track->set_codec_id(codec_id.c_str());

  uint8_t opus_header[OPUS_EXTRADATA_SIZE];
  WebmMuxerUtils::WriteOpusHeader(audio_info.channels.value(),
                                  audio_info.sampling_frequency.value(),
                                  opus_header);
  if (!audio_track->SetCodecPrivate(opus_header, OPUS_EXTRADATA_SIZE))
    LOG(ERROR) << __func__ << ": failed to set opus header.";
}

webm::Status SeekableWebmRemuxer::OnCuePoint(
    const webm::ElementMetadata& metadata,
    const webm::CuePoint& cue_point) {
  // LOG(ERROR) << __FUNCTION__;
  return webm::Callback::OnCuePoint(metadata, cue_point);
}

webm::Status SeekableWebmRemuxer::OnEditionEntry(
    const webm::ElementMetadata& metadata,
    const webm::EditionEntry& edition_entry) {
  // LOG(ERROR) << __FUNCTION__;
  return webm::Callback::OnEditionEntry(metadata, edition_entry);
}

webm::Status SeekableWebmRemuxer::OnTag(const webm::ElementMetadata& metadata,
                                        const webm::Tag& tag) {
  // LOG(ERROR) << __FUNCTION__;
  return webm::Callback::OnTag(metadata, tag);
}

webm::Status SeekableWebmRemuxer::OnSegmentEnd(
    const webm::ElementMetadata& metadata) {
  // LOG(ERROR) << __FUNCTION__;
  return webm::Callback::OnSegmentEnd(metadata);
}

}  // namespace media
