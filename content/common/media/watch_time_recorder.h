// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_MEDIA_WATCH_TIME_RECORDER_H_
#define CONTENT_COMMON_MEDIA_WATCH_TIME_RECORDER_H_

#include <stdint.h>
#include <string>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/time/time.h"
#include "content/common/content_export.h"
#include "media/base/audio_codecs.h"
#include "media/base/video_codecs.h"
#include "media/mojo/interfaces/watch_time_recorder.mojom.h"
#include "url/origin.h"

namespace content {

class CONTENT_EXPORT WatchTimeRecorder
    : public media::mojom::WatchTimeRecorder {
  WatchTimeRecorder(media::AudioCodec audio_codec,
                    media::VideoCodec video_codec,
                    bool is_mse,
                    bool is_eme,
                    const url::Origin& origin);
  ~WatchTimeRecorder() override;

  // media::mojom::WatchTimeRecorder implementation.
  void RecordWatchTime(media::WatchTimeKey key, base::TimeDelta value) override;
  void FinalizeWatchTime(
      const std::vector<media::WatchTimeKey>& watch_time_keys) override;
  void UpdateUnderflowCount(int32_t count) override;

 private:
  // TODO(dalecurtis): Use these values for UKM metrics.
  // const media::AudioCodec audio_codec_;
  // const media::VideoCodec video_codec_;
  // const bool is_mse_;
  // const bool is_eme_;
  const url::Origin origin_;

  // Mapping of WatchTime metric keys to MeanTimeBetweenRebuffers (MTBR) and
  // smooth rate (had zero rebuffers) keys.
  struct RebufferMapping {
    RebufferMapping(const RebufferMapping& copy);
    RebufferMapping(media::WatchTimeKey watch_time_key,
                    base::StringPiece mtbr_key,
                    base::StringPiece smooth_rate_key);
    const media::WatchTimeKey watch_time_key;
    const base::StringPiece mtbr_key;
    const base::StringPiece smooth_rate_key;
  };
  const std::vector<RebufferMapping> rebuffer_keys_;

  base::flat_map<media::WatchTimeKey, base::TimeDelta> watch_time_info_;

  int underflow_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(WatchTimeRecorder);
};

}  // namespace content

#endif  // CONTENT_COMMON_MEDIA_WATCH_TIME_RECORDER_H_
