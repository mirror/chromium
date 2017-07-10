// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/ntp/ntp_tile_saver.h"

#include "base/md5.h"
#include "base/strings/sys_string_conversions.h"
#include "components/ntp_tiles/ntp_tile.h"
#import "ios/chrome/browser/ui/ntp/google_landing_data_source.h"
#import "ios/chrome/browser/ui/ntp/ntp_tile.h"
#include "ios/chrome/common/app_group/app_group_constants.h"
#import "net/base/mac/url_conversions.h"

namespace ntp_tile_saver {
// Returns the path for the temporary favicon folder.
NSURL* tmpFaviconFolderPath() {
  return [[[NSFileManager defaultManager] temporaryDirectory]
      URLByAppendingPathComponent:@"tmpFaviconFolder"];
}

// Replaces the current saved favicons by the contents of the tmp folder.
void replaceSavedFavicons() {
  NSURL* faviconsURL = app_group::ContentWidgetFaviconsFolder();

  if ([[NSFileManager defaultManager] fileExistsAtPath:[faviconsURL path]]) {
    [[NSFileManager defaultManager] removeItemAtURL:faviconsURL error:nil];
  }

  if ([[NSFileManager defaultManager]
          fileExistsAtPath:[tmpFaviconFolderPath() path]]) {
    [[NSFileManager defaultManager] moveItemAtURL:tmpFaviconFolderPath()
                                            toURL:faviconsURL
                                            error:nil];
  }
}

// Checks if every site in |sites| has had its favicons fetched. If so, writes
// the info to disk.
void writeToDiskIfComplete(NSArray<NTPTile*>* sites) {
  for (NTPTile* site : sites) {
    if (!(site.faviconPath || site.fallbackTextColor)) {
      return;
    }
  }

  replaceSavedFavicons();

  NSData* data = [NSKeyedArchiver archivedDataWithRootObject:sites];
  NSUserDefaults* sharedDefaults =
      [[NSUserDefaults alloc] initWithSuiteName:app_group::ApplicationGroup()];
  [sharedDefaults setObject:data forKey:app_group::kSuggestedItems];
}

// Gets a name for the favicon file.
NSString* getFaviconFileName(const ntp_tiles::NTPTile& ntpTile) {
  return [base::SysUTF8ToNSString(base::MD5String(ntpTile.url.spec()))
      stringByAppendingString:@".png"];
}

void saveMostVisitedToDisk(const ntp_tiles::NTPTilesVector& mostVisitedData,
                           id<GoogleLandingDataSource> faviconFetcher) {
  NSMutableArray<NTPTile*>* sites = [[NSMutableArray alloc] init];

  // Clear the tmp favicon folder and recreate it.
  NSURL* tmpFaviconURL = tmpFaviconFolderPath();
  if ([[NSFileManager defaultManager] fileExistsAtPath:[tmpFaviconURL path]]) {
    [[NSFileManager defaultManager] removeItemAtURL:tmpFaviconURL error:nil];
  }
  [[NSFileManager defaultManager] createDirectoryAtPath:[tmpFaviconURL path]
                            withIntermediateDirectories:YES
                                             attributes:nil
                                                  error:nil];

  // For each site, get the favicon. If it is returned, write it to the favicon
  // tmp folder. If a fallback value is returned, update the site info. When the
  // last favicon
  for (const ntp_tiles::NTPTile& ntpTile : mostVisitedData) {
    NTPTile* site =
        [[NTPTile alloc] initWithTitle:base::SysUTF16ToNSString(ntpTile.title)
                                   URL:net::NSURLWithGURL(ntpTile.url)];
    [sites addObject:site];

    NSString* faviconFileName = getFaviconFileName(ntpTile);

    NSURL* fileURL =
        [tmpFaviconURL URLByAppendingPathComponent:faviconFileName];

    void (^faviconImageBlock)(UIImage*) = ^(UIImage* favicon) {
      NSData* imageData = UIImagePNGRepresentation(favicon);
      if ([imageData writeToURL:fileURL atomically:YES]) {
        site.faviconPath = faviconFileName;
      } else {
        // what to do if writing fails? Set a random color?
      }
      writeToDiskIfComplete(sites);
    };

    void (^fallbackBlock)(UIColor*, UIColor*, BOOL) =
        ^(UIColor* textColor, UIColor* backgroundColor, BOOL isDefaultColor) {
          site.fallbackTextColor = textColor;
          site.fallbackBackgroundColor = backgroundColor;
          site.fallbackIsDefaultColor = isDefaultColor;
          writeToDiskIfComplete(sites);
        };

    [faviconFetcher getFaviconForURL:ntpTile.url
                                size:48
                            useCache:NO
                       imageCallback:faviconImageBlock
                    fallbackCallback:fallbackBlock];
  }
}
}
