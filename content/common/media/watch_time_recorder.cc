// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/media/watch_time_recorder.h"

#include "base/metrics/histogram_functions.h"
#include "base/strings/string_piece.h"
#include "components/ukm/public/ukm_entry_builder.h"
#include "components/ukm/public/ukm_recorder.h"
#include "media/base/watch_time_keys.h"

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

}  // namespace

namespace content {

WatchTimeRecorder::WatchTimeRecorder(media::AudioCodec audio_codec,
                                     media::VideoCodec video_codec,
                                     bool is_mse,
                                     bool is_eme,
                                     const url::Origin& origin)
    : origin_(origin),
      watch_time_keys_(media::GetWatchTimeKeys()),
      watch_time_power_keys_(media::GetWatchTimePowerKeys()),
      watch_time_controls_keys_(media::GetWatchTimeControlsKeys()),
      watch_time_display_keys_(media::GetWatchTimeDisplayKeys()),
      rebuffer_keys_(
          {{media::kWatchTimeAudioSrc, media::kMeanTimeBetweenRebuffersAudioSrc,
            media::kRebuffersCountAudioSrc},
           {media::kWatchTimeAudioMse, media::kMeanTimeBetweenRebuffersAudioMse,
            media::kRebuffersCountAudioMse},
           {media::kWatchTimeAudioEme, media::kMeanTimeBetweenRebuffersAudioEme,
            media::kRebuffersCountAudioEme},
           {media::kWatchTimeAudioVideoSrc,
            media::kMeanTimeBetweenRebuffersAudioVideoSrc,
            media::kRebuffersCountAudioVideoSrc},
           {media::kWatchTimeAudioVideoMse,
            media::kMeanTimeBetweenRebuffersAudioVideoMse,
            media::kRebuffersCountAudioVideoMse},
           {media::kWatchTimeAudioVideoEme,
            media::kMeanTimeBetweenRebuffersAudioVideoEme,
            media::kRebuffersCountAudioVideoEme}}) {}

WatchTimeRecorder::~WatchTimeRecorder() {
  FinalizeWatchTime();
}

void WatchTimeRecorder::RecordWatchTime(const std::string& key,
                                        base::TimeDelta value) {
  // Don't log random histogram keys from the untrusted renderer, instead ensure
  // they are from our list of known keys. Use |key_name| from the key map,
  // since otherwise we'll end up storing a StringPiece which points into the
  // soon-to-be-destructed DictionaryValue.
  auto key_name = watch_time_keys_.find(key);
  if (key_name != watch_time_keys_.end())
    watch_time_info_[*key_name] = value;
}

void WatchTimeRecorder::FinalizeWatchTime() {
  // Check for watch times entries that have corresponding MTBR entries
  // and report the MTBR value using watch_time / |underflow_count|
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
  RecordWatchTimeWithFilter(base::flat_set<base::StringPiece>());
}

void WatchTimeRecorder::FinalizePowerWatchTime() {
  RecordWatchTimeWithFilter(watch_time_power_keys_);
}

void WatchTimeRecorder::FinalizeControlsWatchTime() {
  RecordWatchTimeWithFilter(watch_time_controls_keys_);
}

void WatchTimeRecorder::FinalizeDisplayWatchTime() {
  RecordWatchTimeWithFilter(watch_time_display_keys_);
}

void WatchTimeRecorder::UpdateUnderflowCount(int32_t count) {
  underflow_count_ = count;
}

void WatchTimeRecorder::RecordWatchTimeWithFilter(
    const WatchTimeKeys& watch_time_filter) {
  // UKM may be unavailable in content_shell or other non-chrome/ builds; it
  // may also be unavailable if browser shutdown has started.
  const bool has_ukm = !!ukm::UkmRecorder::Get();
  std::unique_ptr<ukm::UkmEntryBuilder> builder;

  for (auto it = watch_time_info_.begin(); it != watch_time_info_.end();) {
    if (!watch_time_filter.empty() &&
        watch_time_filter.find(it->first) == watch_time_filter.end()) {
      ++it;
      continue;
    }

    RecordWatchTimeInternal(it->first, it->second);

    if (has_ukm && ShouldReportUkmWatchTime(it->first)) {
      // if (!builder)
      //   builder = media_internals_->CreateUkmBuilder(url, kWatchTimeEvent);

      // Strip Media.WatchTime. prefix for UKM since they're already
      // grouped; arraysize() includes \0, so no +1 necessary for trailing
      // period.
      builder->AddMetric(it->first.substr(arraysize(kWatchTimeEvent)).data(),
                         it->second.InMilliseconds());
    }

    it = watch_time_info_.erase(it);
  }
}

WatchTimeRecorder::RebufferMapping::RebufferMapping(const RebufferMapping& copy)
    : RebufferMapping(copy.watch_time_key,
                      copy.mtbr_key,
                      copy.smooth_rate_key) {}

WatchTimeRecorder::RebufferMapping::RebufferMapping(
    base::StringPiece watch_time_key,
    base::StringPiece mtbr_key,
    base::StringPiece smooth_rate_key)
    : watch_time_key(watch_time_key),
      mtbr_key(mtbr_key),
      smooth_rate_key(smooth_rate_key) {}

}  // namespace content
