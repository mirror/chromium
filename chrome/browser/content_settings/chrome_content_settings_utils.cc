// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/content_settings/chrome_content_settings_utils.h"

#include "base/metrics/histogram_macros.h"
#include "chrome/browser/media/media_engagement_service.h"
#include "components/content_settings/core/common/content_settings_utils.h"

namespace content_settings {

void RecordMixedScriptAction(MixedScriptAction action) {
  UMA_HISTOGRAM_ENUMERATION("ContentSettings.MixedScript", action,
                            MIXED_SCRIPT_ACTION_COUNT);
}

void RecordPluginsAction(PluginsAction action) {
  UMA_HISTOGRAM_ENUMERATION("ContentSettings.Plugins", action,
                            PLUGINS_ACTION_COUNT);
}

void RecordPopupsAction(PopupsAction action) {
  UMA_HISTOGRAM_ENUMERATION("ContentSettings.Popups", action,
                            POPUPS_ACTION_COUNT);
}

void GetHighMediaEngagementRules(Profile* profile,
                                 RendererContentSettingRules* rules) {
  MediaEngagementService* service = MediaEngagementService::Get(profile);
  bool incognito =
      profile->GetProfileType() == Profile::ProfileType::INCOGNITO_PROFILE;

  for (auto const& pair : service->GetScoreMap()) {
    if (pair.second) {
      ContentSettingsPattern pattern =
          ContentSettingsPattern::FromURL(pair.first);
      rules->high_media_engagement_rules.push_back(ContentSettingPatternSource(
          pattern, pattern, ContentSettingToValue(CONTENT_SETTING_ALLOW),
          std::string(), incognito));
    }
  }

  rules->high_media_engagement_rules.push_back(ContentSettingPatternSource(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingToValue(CONTENT_SETTING_BLOCK), std::string(), incognito));
}

}  // namespace content_settings
