// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ENGAGEMENT_ENGAGEMENT_H_
#define CHROME_BROWSER_ENGAGEMENT_ENGAGEMENT_H_

#include "chrome/browser/engagement/engagement_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"

class HostContentSettingsMap;
class GURL;

std::unique_ptr<base::DictionaryValue> GetScoreDictForSettings(
    const HostContentSettingsMap* settings,
    const GURL& origin_url,
    ContentSettingsType engagement_type) {
  if (!settings)
    return std::make_unique<base::DictionaryValue>();

  std::unique_ptr<base::DictionaryValue> value =
      base::DictionaryValue::From(settings->GetWebsiteSetting(
          origin_url, origin_url, engagement_type,
          content_settings::ResourceIdentifier(), nullptr));

  if (value.get())
    return value;

  return std::make_unique<base::DictionaryValue>();
}

#endif  // CHROME_BROWSER_ENGAGEMENT_ENGAGEMENT_H_
