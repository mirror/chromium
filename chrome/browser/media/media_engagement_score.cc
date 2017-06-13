// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/media_engagement_score.h"

#include "base/memory/ptr_util.h"
#include "base/values.h"
#include "chrome/browser/engagement/site_engagement_metrics.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"

MediaEngagementScore::MediaEngagementScore(const GURL& origin,
                                           HostContentSettingsMap* settings)
    : MediaEngagementScore(
          origin,
          MediaEngagementScore::GetScoreDictForSettings(settings, origin)) {
  settings_map_ = settings;
}

const char* MediaEngagementScore::kVisitsKey = "visits";
const char* MediaEngagementScore::kMediaPlaybacksKey = "mediaPlaybacks";

const int MediaEngagementScore::kScoreMinVisits = 5;

MediaEngagementScore::MediaEngagementScore(
    const GURL& origin,
    std::unique_ptr<base::DictionaryValue> score_dict)
    : media_playbacks_(0),
      visits_(0),
      origin_(origin),
      score_dict_(score_dict.release()) {
  if (!score_dict_)
    return;

  score_dict_->GetInteger(kVisitsKey, &visits_);
  score_dict_->GetInteger(kMediaPlaybacksKey, &media_playbacks_);
}

MediaEngagementScore::MediaEngagementScore(MediaEngagementScore&& other) =
    default;

MediaEngagementScore::~MediaEngagementScore() {}

MediaEngagementScore& MediaEngagementScore::operator=(
    MediaEngagementScore&& other) = default;

double MediaEngagementScore::GetTotalScore() const {
  return MediaEngagementScore::CalculateScore(visits_, media_playbacks_);
}

double MediaEngagementScore::CalculateScore(int visits, int media_playbacks) {
  if (visits < kScoreMinVisits)
    return 0;
  return media_playbacks / (double)visits;
}

void MediaEngagementScore::Commit() {
  DCHECK(settings_map_);
  if (!UpdateScoreDict(score_dict_.get()))
    return;

  settings_map_->SetWebsiteSettingDefaultScope(
      origin_, GURL(), CONTENT_SETTINGS_TYPE_MEDIA_ENGAGEMENT,
      content_settings::ResourceIdentifier(), std::move(score_dict_));
}

bool MediaEngagementScore::UpdateScoreDict(base::DictionaryValue* score_dict) {
  int stored_visits = 0;
  int stored_media_playbacks = 0;

  score_dict->GetInteger(kVisitsKey, &stored_visits);
  score_dict->GetInteger(kMediaPlaybacksKey, &stored_media_playbacks);

  bool changed =
      stored_visits != visits() || stored_media_playbacks != media_playbacks();
  if (!changed)
    return false;

  score_dict->SetInteger(kVisitsKey, visits());
  score_dict->SetInteger(kMediaPlaybacksKey, media_playbacks());

  return true;
}

std::unique_ptr<base::DictionaryValue>
MediaEngagementScore::GetScoreDictForSettings(
    const HostContentSettingsMap* settings,
    const GURL& origin_url) {
  if (!settings)
    return base::MakeUnique<base::DictionaryValue>();

  std::unique_ptr<base::DictionaryValue> value =
      base::DictionaryValue::From(settings->GetWebsiteSetting(
          origin_url, origin_url, CONTENT_SETTINGS_TYPE_MEDIA_ENGAGEMENT,
          content_settings::ResourceIdentifier(), NULL));

  if (value.get())
    return value;
  return base::MakeUnique<base::DictionaryValue>();
}
