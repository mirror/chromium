// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/watch_time_recorder.h"

#include "base/metrics/histogram_functions.h"
#include "base/strings/string_piece.h"
#include "media/base/watch_time_keys.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/metrics/public/cpp/ukm_entry_builder.h"
#include "services/metrics/public/cpp/ukm_recorder.h"

namespace media {

static void RecordWatchTimeInternal(base::StringPiece key,
                                    base::TimeDelta value,
                                    bool is_mtbr = false) {
  base::UmaHistogramCustomTimes(
      key.as_string(), value,
      // There are a maximum of 5 underflow events possible in a given 7s
      // watch time period, so the minimum value is 1.4s.
      is_mtbr ? base::TimeDelta::FromSecondsD(1.4)
              : base::TimeDelta::FromSeconds(7),
      base::TimeDelta::FromHours(10), 50);
}

static void RecordMeanTimeBetweenRebuffers(base::StringPiece key,
                                           base::TimeDelta value) {
  RecordWatchTimeInternal(key, value, true);
}

static void RecordRebuffersCount(base::StringPiece key, int underflow_count) {
  base::UmaHistogramCounts100(key.as_string(), underflow_count);
}

class WatchTimeRecorderProvider : public mojom::WatchTimeRecorderProvider {
 public:
  WatchTimeRecorderProvider() {}
  ~WatchTimeRecorderProvider() override {}

 private:
  // mojom::WatchTimeRecorderProvider implementation:
  void AcquireWatchTimeRecorder(
      mojom::PlaybackPropertiesPtr properties,
      mojom::WatchTimeRecorderRequest request) override {
    mojo::MakeStrongBinding(
        base::MakeUnique<WatchTimeRecorder>(std::move(properties)),
        std::move(request));
  }

  DISALLOW_COPY_AND_ASSIGN(WatchTimeRecorderProvider);
};

const char WatchTimeRecorder::kWatchTimeUkmEvent[] = "Media.BasicPlayback";

WatchTimeRecorder::WatchTimeRecorder(mojom::PlaybackPropertiesPtr properties)
    : properties_(std::move(properties)),
      rebuffer_keys_(
          {{WatchTimeKey::kAudioSrc, kMeanTimeBetweenRebuffersAudioSrc,
            kRebuffersCountAudioSrc},
           {WatchTimeKey::kAudioMse, kMeanTimeBetweenRebuffersAudioMse,
            kRebuffersCountAudioMse},
           {WatchTimeKey::kAudioEme, kMeanTimeBetweenRebuffersAudioEme,
            kRebuffersCountAudioEme},
           {WatchTimeKey::kAudioVideoSrc,
            kMeanTimeBetweenRebuffersAudioVideoSrc,
            kRebuffersCountAudioVideoSrc},
           {WatchTimeKey::kAudioVideoMse,
            kMeanTimeBetweenRebuffersAudioVideoMse,
            kRebuffersCountAudioVideoMse},
           {WatchTimeKey::kAudioVideoEme,
            kMeanTimeBetweenRebuffersAudioVideoEme,
            kRebuffersCountAudioVideoEme}}) {}

WatchTimeRecorder::~WatchTimeRecorder() {
  FinalizeWatchTime({});
}

// static
void WatchTimeRecorder::CreateWatchTimeRecorderProvider(
    mojom::WatchTimeRecorderProviderRequest request) {
  mojo::MakeStrongBinding(base::MakeUnique<WatchTimeRecorderProvider>(),
                          std::move(request));
}

void WatchTimeRecorder::RecordWatchTime(WatchTimeKey key,
                                        base::TimeDelta watch_time) {
  watch_time_info_[key] = watch_time;
}

void WatchTimeRecorder::FinalizeWatchTime(
    const std::vector<WatchTimeKey>& keys_to_finalize) {
  // If the filter set is empty, treat that as finalizing all keys; otherwise
  // only the listed keys will be finalized.
  const bool should_finalize_everything = keys_to_finalize.empty();

  // Only report underflow count when finalizing everything.
  if (should_finalize_everything) {
    // This must be done before |underflow_count_| is cleared.
    RecordUkmPlaybackData();

    // Check for watch times entries that have corresponding MTBR entries and
    // report the MTBR value using watch_time / |underflow_count|.
    for (auto& mapping : rebuffer_keys_) {
      auto it = watch_time_info_.find(mapping.watch_time_key);
      if (it == watch_time_info_.end())
        continue;

      if (underflow_count_) {
        RecordMeanTimeBetweenRebuffers(mapping.mtbr_key,
                                       it->second / underflow_count_);
      }

      RecordRebuffersCount(mapping.smooth_rate_key, underflow_count_);
    }

    underflow_count_ = 0;
  }

  for (auto it = watch_time_info_.begin(); it != watch_time_info_.end();) {
    if (!should_finalize_everything &&
        std::find(keys_to_finalize.begin(), keys_to_finalize.end(),
                  it->first) == keys_to_finalize.end()) {
      ++it;
      continue;
    }

    const base::StringPiece metric_name = WatchTimeKeyToString(it->first);
    RecordWatchTimeInternal(metric_name, it->second);
    it = watch_time_info_.erase(it);
  }
}

void WatchTimeRecorder::OnError(PipelineStatus status) {
  pipeline_status_ = status;
}

void WatchTimeRecorder::UpdateUnderflowCount(int32_t count) {
  underflow_count_ = count;
}

void WatchTimeRecorder::RecordUkmPlaybackData() {
  // Ensure we have an "All" watch time entry or skip reporting.
  bool have_watch_time_all = false;
  for (auto& kv : watch_time_info_) {
    if (kv.first == WatchTimeKey::kAudioAll ||
        kv.first == WatchTimeKey::kAudioVideoAll ||
        kv.first == WatchTimeKey::kAudioVideoBackgroundAll) {
      have_watch_time_all = true;
      break;
    }
  }
  if (!have_watch_time_all)
    return;

  // UKM may be unavailable in content_shell or other non-chrome/ builds; it
  // may also be unavailable if browser shutdown has started; so this may be a
  // nullptr. If it's unavailable, UKM reporting will be skipped.
  ukm::UkmRecorder* ukm_recorder = ukm::UkmRecorder::Get();
  if (!ukm_recorder)
    return;

  const int32_t source_id = ukm_recorder->GetNewSourceID();
  ukm_recorder->UpdateSourceURL(source_id, properties_->origin.GetURL());
  std::unique_ptr<ukm::UkmEntryBuilder> builder =
      ukm_recorder->GetEntryBuilder(source_id, kWatchTimeUkmEvent);

  for (auto& kv : watch_time_info_) {
    if (kv.first == WatchTimeKey::kAudioAll ||
        kv.first == WatchTimeKey::kAudioVideoAll ||
        kv.first == WatchTimeKey::kAudioVideoBackgroundAll) {
      builder->AddMetric("WatchTime", kv.second.InMilliseconds());
      if (underflow_count_) {
        builder->AddMetric("MeanTimeBetweenRebuffers",
                           (kv.second / underflow_count_).InMilliseconds());
      }
      builder->AddMetric("IsBackground",
                         kv.first == WatchTimeKey::kAudioVideoBackgroundAll);
    } else if (kv.first == WatchTimeKey::kAudioAc ||
               kv.first == WatchTimeKey::kAudioVideoAc ||
               kv.first == WatchTimeKey::kAudioVideoBackgroundAc) {
      builder->AddMetric("WatchTime.AC", kv.second.InMilliseconds());
    } else if (kv.first == WatchTimeKey::kAudioBattery ||
               kv.first == WatchTimeKey::kAudioVideoBattery ||
               kv.first == WatchTimeKey::kAudioVideoBackgroundBattery) {
      builder->AddMetric("WatchTime.Battery", kv.second.InMilliseconds());
    } else if (kv.first == WatchTimeKey::kAudioNativeControlsOn ||
               kv.first == WatchTimeKey::kAudioVideoNativeControlsOn) {
      builder->AddMetric("WatchTime.NativeControlsOn",
                         kv.second.InMilliseconds());
    } else if (kv.first == WatchTimeKey::kAudioNativeControlsOff ||
               kv.first == WatchTimeKey::kAudioVideoNativeControlsOff) {
      builder->AddMetric("WatchTime.NativeControlsOff",
                         kv.second.InMilliseconds());
    } else if (kv.first == WatchTimeKey::kAudioVideoDisplayFullscreen) {
      builder->AddMetric("WatchTime.DisplayFullscreen",
                         kv.second.InMilliseconds());
    } else if (kv.first == WatchTimeKey::kAudioVideoDisplayInline) {
      builder->AddMetric("WatchTime.DisplayInline", kv.second.InMilliseconds());
    } else if (kv.first == WatchTimeKey::kAudioVideoDisplayPictureInPicture) {
      builder->AddMetric("WatchTime.DisplayPictureInPicture",
                         kv.second.InMilliseconds());
    }
  }

  // See note in mojom::PlaybackProperties about why we have both of these.
  builder->AddMetric("AudioCodec", properties_->audio_codec);
  builder->AddMetric("VideoCodec", properties_->video_codec);
  builder->AddMetric("HasAudio", properties_->has_audio);
  builder->AddMetric("HasVideo", properties_->has_video);

  builder->AddMetric("IsEME", properties_->is_eme);
  builder->AddMetric("IsMSE", properties_->is_mse);
  builder->AddMetric("PipelineStatus", pipeline_status_);
  builder->AddMetric("RebuffersCount", underflow_count_);
  builder->AddMetric("VideoNaturalWidth", properties_->natural_size.width());
  builder->AddMetric("VideoNaturalHeight", properties_->natural_size.height());
}

WatchTimeRecorder::RebufferMapping::RebufferMapping(const RebufferMapping& copy)
    : RebufferMapping(copy.watch_time_key,
                      copy.mtbr_key,
                      copy.smooth_rate_key) {}

WatchTimeRecorder::RebufferMapping::RebufferMapping(
    WatchTimeKey watch_time_key,
    base::StringPiece mtbr_key,
    base::StringPiece smooth_rate_key)
    : watch_time_key(watch_time_key),
      mtbr_key(mtbr_key),
      smooth_rate_key(smooth_rate_key) {}

}  // namespace media
