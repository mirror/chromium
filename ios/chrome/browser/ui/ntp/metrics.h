// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_NTP_METRICS_H_
#define IOS_CHROME_BROWSER_UI_NTP_METRICS_H_

#include "components/ntp_tiles/ntp_tile.h"
#import "ios/chrome/browser/ui/favicon/favicon_attributes.h"

void RecordNtpTileImpression(int index, const ntp_tiles::NTPTile& tile,
                          const FaviconAttributes* attributes);


void RecordNtpTileClick(int index, const ntp_tiles::NTPTile& tile,
                        const FaviconAttributes* attributes);

#endif  // IOS_CHROME_BROWSER_UI_NTP_METRICS_H_
