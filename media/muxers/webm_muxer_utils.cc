// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/muxers/webm_muxer_utils.h"

#include "media/filters/opus_constants.h"
#include "media/muxers/seekable_webm_remuxer.h"
#include "media/muxers/webm_reader_adapter.h"
#include "third_party/libwebm/source/webm_parser/include/webm/webm_parser.h"

namespace media {

void WebmMuxerUtils::WriteOpusHeader(int channel_count, int sample_rate_in_hz,
                                     uint8_t* header) {
  // See https://wiki.xiph.org/OggOpus#ID_Header.
  // Set magic signature.
  std::string label = "OpusHead";
  memcpy(header + OPUS_EXTRADATA_LABEL_OFFSET, label.c_str(), label.size());
  // Set Opus version.
  header[OPUS_EXTRADATA_VERSION_OFFSET] = 1;
  // Set channel count.
  header[OPUS_EXTRADATA_CHANNELS_OFFSET] = channel_count;
  // Set pre-skip
  uint16_t skip = 0;
  memcpy(header + OPUS_EXTRADATA_SKIP_SAMPLES_OFFSET, &skip, sizeof(uint16_t));
  // Set original input sample rate in Hz.
  uint32_t sample_rate = sample_rate_in_hz;
  memcpy(header + OPUS_EXTRADATA_SAMPLE_RATE_OFFSET, &sample_rate,
         sizeof(uint32_t));
  // Set output gain in dB.
  uint16_t gain = 0;
  memcpy(header + OPUS_EXTRADATA_GAIN_OFFSET, &gain, 2);

  // Set channel mapping.
  if (channel_count > 2) {
    // Also possible to have a multistream, not supported for now.
    DCHECK_LE(channel_count, OPUS_MAX_VORBIS_CHANNELS);
    header[OPUS_EXTRADATA_CHANNEL_MAPPING_OFFSET] = 1;
    // Assuming no coupled streams. This should actually be
    // channels() - |coupled_streams|.
    header[OPUS_EXTRADATA_NUM_STREAMS_OFFSET] = channel_count;
    header[OPUS_EXTRADATA_NUM_COUPLED_OFFSET] = 0;
    // Set the actual stream map.
    for (int i = 0; i < channel_count; ++i) {
      header[OPUS_EXTRADATA_STREAM_MAP_OFFSET + i] =
          kOpusVorbisChannelMap[channel_count - 1][i];
    }
  } else {
    header[OPUS_EXTRADATA_CHANNEL_MAPPING_OFFSET] = 0;
  }
}

bool WebmMuxerUtils::ConvertLiveModeRecordingToFileModeRecording(
    std::unique_ptr<MkvReader> live_mode_recording_buffer,
    MkvReaderWriter* file_mode_buffer,
    MkvReaderWriter* cues_before_clusters_buffer_) {
  // Create a WebmParser and a SeekableWebmWritingCallback.
  webm::WebmParser parser;
  SeekableWebmRemuxer remuxer(file_mode_buffer);
  // Feed the WebmParser with the recording buffer.
  WebmReaderAdapter reader_adapter(live_mode_recording_buffer.get());
  parser.Feed(&remuxer, &reader_adapter);
  if (remuxer.has_error()) {
    LOG(ERROR) << "SeekableWebmWritingCallback error during muxing";
    return false;
  }
  // The source buffer is no longer needed.
  live_mode_recording_buffer.reset();

  if (!remuxer.Finalize()) {
    LOG(ERROR) << "SeekableWebmWritingCallback error during finalize";
    return false;
  }

  // Perform the CopyAndMoveCuesBeforeClusters into a new buffer.
  LOG(ERROR) << "Creating indexed WebM file data";
  return remuxer.CopyAndMoveCuesBeforeClusters(cues_before_clusters_buffer_);
}

}  // namespace media
