// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_WATCH_TIME_RECORDER_H_
#define MEDIA_MOJO_SERVICES_WATCH_TIME_RECORDER_H_

#include <stdint.h>
#include <string>

#include "base/compiler_specific.h"
#include "base/containers/flat_map.h"
#include "base/time/time.h"
#include "media/base/audio_codecs.h"
#include "media/base/pipeline_status.h"
#include "media/base/video_codecs.h"
#include "media/mojo/interfaces/watch_time_recorder.mojom.h"
#include "media/mojo/services/media_mojo_export.h"
#include "url/origin.h"

namespace media {

// See mojom::WatchTimeRecorder for documentation.
class MEDIA_MOJO_EXPORT WatchTimeRecorder : public mojom::WatchTimeRecorder {
 public:
  explicit WatchTimeRecorder(mojom::PlaybackPropertiesPtr properties);
  ~WatchTimeRecorder() override;

  static void CreateWatchTimeRecorderProvider(
      mojom::WatchTimeRecorderProviderRequest request);

  // mojom::WatchTimeRecorder implementation:
  void RecordWatchTime(WatchTimeKey key, base::TimeDelta watch_time) override;
  void FinalizeWatchTime(
      const std::vector<WatchTimeKey>& watch_time_keys) override;
  void OnError(PipelineStatus status) override;
  void UpdateUnderflowCount(int32_t count) override;

  // UKM key strings exposed for tests only.
  static const char kWatchTimeUkmEvent[];

  // TODO(dalecurtis): If we get too many of these, switch to a suffix based
  // concatenation system.
  static const char kUkmKeyWatchTime[];
  static const char kUkmKeyWatchTimeAc[];
  static const char kUkmKeyWatchTimeBattery[];
  static const char kUkmKeyWatchTimeNativeControlsOn[];
  static const char kUkmKeyWatchTimeNativeControlsOff[];
  static const char kUkmKeyWatchTimeDisplayFullscreen[];
  static const char kUkmKeyWatchTimeDisplayInline[];
  static const char kUkmKeyWatchTimeDisplayPictureInPicture[];

  static const char kUkmKeyMeanTimeBetweenRebuffers[];
  static const char kUkmKeyIsBackground[];
  static const char kUkmKeyAudioCodec[];
  static const char kUkmKeyVideoCodec[];
  static const char kUkmKeyHasAudio[];
  static const char kUkmKeyHasVideo[];
  static const char kUkmKeyIsEME[];
  static const char kUkmKeyIsMSE[];
  static const char kUkmKeyLastPipelineStatus[];
  static const char kUkmKeyRebuffersCount[];
  static const char kUkmKeyVideoNaturalWidth[];
  static const char kUkmKeyVideoNaturalHeight[];

 private:
  void RecordUkmPlaybackData(const std::vector<WatchTimeKey>& keys_to_finalize);

  const mojom::PlaybackPropertiesPtr properties_;

  // Mapping of WatchTime metric keys to MeanTimeBetweenRebuffers (MTBR) and
  // smooth rate (had zero rebuffers) keys.
  struct RebufferMapping {
    RebufferMapping(const RebufferMapping& copy);
    RebufferMapping(WatchTimeKey watch_time_key,
                    base::StringPiece mtbr_key,
                    base::StringPiece smooth_rate_key);
    const WatchTimeKey watch_time_key;
    const base::StringPiece mtbr_key;
    const base::StringPiece smooth_rate_key;
  };
  const std::vector<RebufferMapping> rebuffer_keys_;

  base::flat_map<WatchTimeKey, base::TimeDelta> watch_time_info_;

  int underflow_count_ = 0;
  PipelineStatus pipeline_status_ = PIPELINE_OK;

  DISALLOW_COPY_AND_ASSIGN(WatchTimeRecorder);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_WATCH_TIME_RECORDER_H_
