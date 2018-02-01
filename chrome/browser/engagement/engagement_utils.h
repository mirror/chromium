// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ENGAGEMENT_ENGAGEMENT_UTILS_H_
#define CHROME_BROWSER_ENGAGEMENT_ENGAGEMENT_UTILS_H_

#include <memory>

#include "components/content_settings/core/browser/host_content_settings_map.h"

std::unique_ptr<base::DictionaryValue> GetScoreDictForSettings(
    const HostContentSettingsMap* settings,
    const GURL& origin_url,
    ContentSettingsType engagement_type);

#endif  // CHROME_BROWSER_ENGAGEMENT_ENGAGEMENT_UTILS_H_
