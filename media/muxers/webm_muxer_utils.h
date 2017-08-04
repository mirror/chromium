// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MUXERS_WEBM_MUXER_UTILS_H_
#define MEDIA_MUXERS_WEBM_MUXER_UTILS_H_

#include <cstdint>
#include <memory>

#include "base/macros.h"
#include "media/base/audio_parameters.h"
#include "media/base/media_export.h"
#include "media/muxers/mkv_reader_writer.h"

namespace media {

class MEDIA_EXPORT WebmMuxerUtils {
 public:
  static void WriteOpusHeader(int channel_count, int sample_rate_in_hz,
                              uint8_t* header);

  static bool ConvertLiveModeRecordingToFileModeRecording(
    std::unique_ptr<MkvReader> live_mode_recording_buffer,
    MkvReaderWriter* file_mode_buffer,
    MkvReaderWriter* cues_before_clusters_buffer_);
};

}  // namespace media

#endif  // MEDIA_MUXERS_WEBM_MUXER_UTILS_H_
