// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/content_settings/sound_content_setting_observer.h"

#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/browser/navigation_handle.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(SoundContentSettingObserver);

SoundContentSettingObserver::SoundContentSettingObserver(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      web_contents_(web_contents),
      observer_(this) {
  DCHECK(web_contents_);
  host_content_settings_map_ = HostContentSettingsMapFactory::GetForProfile(
      Profile::FromBrowserContext(web_contents_->GetBrowserContext()));
  observer_.Add(host_content_settings_map_);
}

SoundContentSettingObserver::~SoundContentSettingObserver() {}

void SoundContentSettingObserver::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (navigation_handle->IsInMainFrame() && navigation_handle->HasCommitted() &&
      !navigation_handle->IsSameDocument()) {
    MuteOrUnmuteIfNecessary();
  }
}

void SoundContentSettingObserver::OnContentSettingChanged(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type,
    std::string resource_identifier) {
  if (content_type == CONTENT_SETTINGS_TYPE_SOUND)
    MuteOrUnmuteIfNecessary();
}

void SoundContentSettingObserver::MuteOrUnmuteIfNecessary() {
  web_contents_->SetAudioMuted(GetCurrentContentSetting() ==
                               CONTENT_SETTING_BLOCK);
}

ContentSetting SoundContentSettingObserver::GetCurrentContentSetting() {
  GURL url = web_contents_->GetURL();
  ContentSetting setting = host_content_settings_map_->GetContentSetting(
      url, url, CONTENT_SETTINGS_TYPE_SOUND, std::string());
  if (setting == CONTENT_SETTING_DEFAULT) {
    return host_content_settings_map_->GetDefaultContentSetting(
        CONTENT_SETTINGS_TYPE_SOUND, nullptr);
  }
  return setting;
}
