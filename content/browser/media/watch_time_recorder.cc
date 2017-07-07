// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/watch_time_recorder.h"

#include "base/metrics/histogram_functions.h"
#include "base/strings/string_piece.h"
#include "media/base/watch_time_keys.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/metrics/public/cpp/ukm_entry_builder.h"
#include "services/metrics/public/cpp/ukm_recorder.h"

namespace {

const char kWatchTimeEvent[] = "Media.WatchTime";

void RecordWatchTimeInternal(base::StringPiece key,
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

void RecordMeanTimeBetweenRebuffers(base::StringPiece key,
                                    base::TimeDelta value) {
  RecordWatchTimeInternal(key, value, true);
}

void RecordRebuffersCount(base::StringPiece key, int underflow_count) {
  base::UmaHistogramCounts100(key.as_string(), underflow_count);
}

bool ShouldReportUkmWatchTime(base::StringPiece key) {
  // EmbeddedExperience is always a file:// URL, so skip reporting.
  return !key.ends_with("EmbeddedExperience");
}

class WatchTimeRecorderProvider
    : public media::mojom::WatchTimeRecorderProvider {
 public:
  WatchTimeRecorderProvider() {}
  ~WatchTimeRecorderProvider() override {}

 private:
  // media::mojom::WatchTimeRecorderProvider implementation:
  void AcquireWatchTimeRecorder(
      media::AudioCodec audio_codec,
      media::VideoCodec video_codec,
      bool is_mse,
      bool is_eme,
      const gfx::Size& video_size,
      const url::Origin& origin,
      media::mojom::WatchTimeRecorderRequest request) override {
    mojo::MakeStrongBinding(
        base::MakeUnique<content::WatchTimeRecorder>(
            audio_codec, video_codec, is_mse, is_eme, video_size, origin),
        std::move(request));
  }

  DISALLOW_COPY_AND_ASSIGN(WatchTimeRecorderProvider);
};

}  // namespace

namespace content {

// TODO(dalecurtis): Use these values in UKM reports.
WatchTimeRecorder::WatchTimeRecorder(media::AudioCodec audio_codec,
                                     media::VideoCodec video_codec,
                                     bool is_mse,
                                     bool is_eme,
                                     const gfx::Size& video_size,
                                     const url::Origin& origin)
    : origin_(origin),
      rebuffer_keys_({{media::WatchTimeKey::kAudioSrc,
                       media::kMeanTimeBetweenRebuffersAudioSrc,
                       media::kRebuffersCountAudioSrc},
                      {media::WatchTimeKey::kAudioMse,
                       media::kMeanTimeBetweenRebuffersAudioMse,
                       media::kRebuffersCountAudioMse},
                      {media::WatchTimeKey::kAudioEme,
                       media::kMeanTimeBetweenRebuffersAudioEme,
                       media::kRebuffersCountAudioEme},
                      {media::WatchTimeKey::kAudioVideoSrc,
                       media::kMeanTimeBetweenRebuffersAudioVideoSrc,
                       media::kRebuffersCountAudioVideoSrc},
                      {media::WatchTimeKey::kAudioVideoMse,
                       media::kMeanTimeBetweenRebuffersAudioVideoMse,
                       media::kRebuffersCountAudioVideoMse},
                      {media::WatchTimeKey::kAudioVideoEme,
                       media::kMeanTimeBetweenRebuffersAudioVideoEme,
                       media::kRebuffersCountAudioVideoEme}}) {}

WatchTimeRecorder::~WatchTimeRecorder() {
  FinalizeWatchTime({});
}

// static
void WatchTimeRecorder::CreateWatchTimeRecorderProvider(
    const service_manager::BindSourceInfo& source_info,
    media::mojom::WatchTimeRecorderProviderRequest request) {
  mojo::MakeStrongBinding(base::MakeUnique<WatchTimeRecorderProvider>(),
                          std::move(request));
}

void WatchTimeRecorder::RecordWatchTime(media::WatchTimeKey key,
                                        base::TimeDelta value) {
  watch_time_info_[key] = value;
}

void WatchTimeRecorder::FinalizeWatchTime(
    const std::vector<media::WatchTimeKey>& keys_to_finalize) {
  // If the filter set is empty, treat that as finalizing all keys; otherwise
  // only the listed keys will be finalized.
  const bool should_finalize_everything = keys_to_finalize.empty();

  // Only report underflow count when finalizing everything.
  if (should_finalize_everything) {
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

  // UKM may be unavailable in content_shell or other non-chrome/ builds; it
  // may also be unavailable if browser shutdown has started; so this may be a
  // nullptr. If it's unavailable, UKM reporting will be skipped.
  ukm::UkmRecorder* ukm_recorder = ukm::UkmRecorder::Get();
  std::unique_ptr<ukm::UkmEntryBuilder> builder;

  for (auto it = watch_time_info_.begin(); it != watch_time_info_.end();) {
    if (!should_finalize_everything &&
        std::find(keys_to_finalize.begin(), keys_to_finalize.end(),
                  it->first) == keys_to_finalize.end()) {
      ++it;
      continue;
    }

    const base::StringPiece metric_name =
        media::WatchTimeKeyToString(it->first);
    RecordWatchTimeInternal(metric_name, it->second);

    if (ukm_recorder && ShouldReportUkmWatchTime(metric_name)) {
      if (!builder) {
        const int32_t source_id = ukm_recorder->GetNewSourceID();
        ukm_recorder->UpdateSourceURL(source_id, origin_.GetURL());
        builder = ukm_recorder->GetEntryBuilder(source_id, kWatchTimeEvent);
      }

      // Strip Media.WatchTime. prefix for UKM since they're already grouped;
      // arraysize() includes \0, so no +1 necessary for trailing period.
      builder->AddMetric(metric_name.substr(arraysize(kWatchTimeEvent)).data(),
                         it->second.InMilliseconds());
    }

    it = watch_time_info_.erase(it);
  }
}

void WatchTimeRecorder::UpdateUnderflowCount(int32_t count) {
  underflow_count_ = count;
}

WatchTimeRecorder::RebufferMapping::RebufferMapping(const RebufferMapping& copy)
    : RebufferMapping(copy.watch_time_key,
                      copy.mtbr_key,
                      copy.smooth_rate_key) {}

WatchTimeRecorder::RebufferMapping::RebufferMapping(
    media::WatchTimeKey watch_time_key,
    base::StringPiece mtbr_key,
    base::StringPiece smooth_rate_key)
    : watch_time_key(watch_time_key),
      mtbr_key(mtbr_key),
      smooth_rate_key(smooth_rate_key) {}

}  // namespace content
