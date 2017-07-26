// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/content_settings/content_settings_update_observer.h"

#include "base/metrics/histogram_macros.h"
#include "base/values.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"

namespace content_settings {

ContentSettingsUpdateObserver::ContentSettingsUpdateObserver(
    content::WebContents* tab)
    : binding_(tab, this) {}

ContentSettingsUpdateObserver::~ContentSettingsUpdateObserver() {}

void ContentSettingsUpdateObserver::UpdateContentSetting(
    const ContentSettingsPattern& primary_pattern,
    std::unique_ptr<base::DictionaryValue> expiration_times) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  content::RenderProcessHost* rph =
      binding_.GetCurrentTargetFrame()->GetProcess();
  content::BrowserContext* browser_context = rph->GetBrowserContext();
  Profile* profile = Profile::FromBrowserContext(browser_context);

  HostContentSettingsMap* map =
      HostContentSettingsMapFactory::GetForProfile(profile);
  if (!map)
    return;

  // TODO (tbansal): crbug.com/735518. Disable updates to client hints settings
  // when cookies are disabled for the origin corresponding to
  // |primary_pattern|.
  map->SetWebsiteSettingCustomScope(
      primary_pattern, ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_CLIENT_HINTS, std::string(),
      expiration_times->CreateDeepCopy());

  UMA_HISTOGRAM_COUNTS_100("ContentSettings.ClientHints.UpdateSize",
                           expiration_times->size());
}

}  // namespace content_settings

DEFINE_WEB_CONTENTS_USER_DATA_KEY(
    content_settings::ContentSettingsUpdateObserver);
