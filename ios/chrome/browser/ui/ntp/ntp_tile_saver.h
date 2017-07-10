// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_NTP_NTP_TILE_SAVER_H_
#define IOS_CHROME_BROWSER_UI_NTP_NTP_TILE_SAVER_H_

#import <Foundation/Foundation.h>

#include "components/ntp_tiles/ntp_tile.h"

@protocol GoogleLandingDataSource;
@class NTPTile;

// These functions are used to save the ntp tiles (favicon and name) offline for
// the use of the content widget. The most visited info and icon fallback data
// are saved to userdefaults. The favicons are saved to a shared folder. Because
// the favicons are fetched asynchronously, they are first saved in a temporary
// folder which replaces the current favicons when all are fetched.
namespace ntp_tile_saver {

// Returns the path for the temporary favicon folder.
NSURL* tmpFaviconFolderPath();

// Replaces the current saved favicons by the contents of the tmp folder.
void replaceSavedFavicons();

// Checks if every site in |sites| has had its favicons fetched. If so, writes
// the info to disk.
void writeToDiskIfComplete(NSArray<NTPTile*>* sites);

// Gets a name for the favicon file.
NSString* getFaviconFileName(const ntp_tiles::NTPTile& ntpTile);

// Saves the most visited sites to disk, using |faviconFetcher| to get the
// favicons.
void saveMostVisitedToDisk(const ntp_tiles::NTPTilesVector& mostVisitedData,
                           id<GoogleLandingDataSource> faviconFetcher);

}  // namespace ntp_tile_saver

#endif  // IOS_CHROME_BROWSER_UI_NTP_NTP_TILE_SAVER_H_
