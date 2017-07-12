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
NSURL* TmpFaviconFolderPath();

// Replaces the current saved favicons by the contents of the tmp folder.
void ReplaceSavedFavicons();

// Checks if every site in |tiles| has had its favicons fetched. If so, writes
// the info to disk.
void WriteToDiskIfComplete(NSArray<NTPTile*>* tiles);

// Gets a name for the favicon file.
NSString* GetFaviconFileName(const ntp_tiles::NTPTile& ntpTile);

// Saves the most visited sites to disk, using |faviconFetcher| to get the
// favicons.
void SaveMostVisitedToDisk(const ntp_tiles::NTPTilesVector& mostVisitedData,
                           id<GoogleLandingDataSource> faviconFetcher);

// Write the |mostVisitedSites| to disk.
void WriteSavedMostVisited(NSDictionary<NSURL*, NTPTile*>* mostVisitedSites);

// Read the current saved most visited sites from disk.
NSDictionary* ReadSavedMostVisited();

// If the sites currently saved include one with |tile|'s url, replace it by
// |tile|.
void WriteSingleUpdatedTileToDisk(NTPTile* tile);

// Fetches and saves to disk the updated favicon for a single site.
void UpdateSingleFavicon(const GURL& siteURL,
                         id<GoogleLandingDataSource> faviconFetcher);

}  // namespace ntp_tile_saver

#endif  // IOS_CHROME_BROWSER_UI_NTP_NTP_TILE_SAVER_H_
