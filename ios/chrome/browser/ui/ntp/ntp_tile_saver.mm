// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/ntp/ntp_tile_saver.h"

#include "base/bind.h"
#import "base/mac/bind_objc_block.h"
#include "base/md5.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "components/favicon/core/fallback_url_util.h"
#include "components/ntp_tiles/ntp_tile.h"
#import "ios/chrome/browser/ui/ntp/google_landing_data_source.h"
#import "ios/chrome/browser/ui/ntp/ntp_tile.h"
#include "ios/chrome/common/app_group/app_group_constants.h"
#import "net/base/mac/url_conversions.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace ntp_tile_saver {

// Write the |most_visited_sites| to disk.
void WriteSavedMostVisited(NSDictionary<NSURL*, NTPTile*>* most_visited_sites);

// Returns the path for the temporary favicon folder.
NSURL* GetTemporaryFaviconFolderPath();

// Replaces the current saved favicons at |favicons_url| by the contents of the
// tmp folder.
void ReplaceSavedFavicons(NSURL* favicons_url);

// Checks if every site in |tiles| has had its favicons fetched. If so, writes
// the info to disk, saving the favicons to |favicons_url|.
void WriteToDiskIfComplete(NSDictionary<NSURL*, NTPTile*>* tiles,
                           NSURL* favicons_url);

// Gets a name for the favicon file.
NSString* GetFaviconFileName(const GURL& url);

// If the sites currently saved include one with |tile|'s url, replace it by
// |tile|.
void WriteSingleUpdatedTileToDisk(NTPTile* tile);

// Empty the temporary favicon folder.
void ClearTemporaryFaviconFolder();

// Get the favicons using |favicon_fetcher| and writes them to disk.
void GetFaviconsAndSave(const ntp_tiles::NTPTilesVector& most_visited_data,
                        id<GoogleLandingDataSource> favicon_fetcher,
                        NSURL* favicons_url);

}  // namespace ntp_tile_saver

namespace ntp_tile_saver {

NSURL* GetTemporaryFaviconFolderPath() {
  return [[NSURL fileURLWithPath:NSTemporaryDirectory()]
      URLByAppendingPathComponent:@"tmpFaviconFolder"];
}

void ReplaceSavedFavicons(NSURL* favicons_url) {
  if ([[NSFileManager defaultManager] fileExistsAtPath:[favicons_url path]]) {
    [[NSFileManager defaultManager] removeItemAtURL:favicons_url error:nil];
  }

  if ([[NSFileManager defaultManager]
          fileExistsAtPath:[GetTemporaryFaviconFolderPath() path]]) {
    [[NSFileManager defaultManager]
               createDirectoryAtURL:favicons_url.URLByDeletingLastPathComponent
        withIntermediateDirectories:YES
                         attributes:nil
                              error:nil];

    [[NSFileManager defaultManager]
        moveItemAtURL:GetTemporaryFaviconFolderPath()
                toURL:favicons_url
                error:nil];
  }
}

void WriteToDiskIfComplete(NSDictionary<NSURL*, NTPTile*>* tiles,
                           NSURL* favicons_url) {
  for (NSURL* siteURL : tiles) {
    NTPTile* tile = [tiles objectForKey:siteURL];
    if (!tile.faviconFetched) {
      return;
    }
    // Any fetched tile must have a file name or a fallback monogram.
    DCHECK(tile.faviconFileName || tile.fallbackMonogram);
  }

  base::PostTaskWithTraitsAndReply(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::BindBlockArc(^{
        ReplaceSavedFavicons(favicons_url);
      }),
      base::BindBlockArc(^{
        WriteSavedMostVisited(tiles);
      }));
}

void ClearTemporaryFaviconFolder() {
  NSURL* tmpFaviconURL = GetTemporaryFaviconFolderPath();
  if ([[NSFileManager defaultManager] fileExistsAtPath:[tmpFaviconURL path]]) {
    [[NSFileManager defaultManager] removeItemAtURL:tmpFaviconURL error:nil];
  }
  [[NSFileManager defaultManager] createDirectoryAtPath:[tmpFaviconURL path]
                            withIntermediateDirectories:YES
                                             attributes:nil
                                                  error:nil];
}

NSString* GetFaviconFileName(const GURL& url) {
  return [base::SysUTF8ToNSString(base::MD5String(url.spec()))
      stringByAppendingString:@".png"];
}

void GetFaviconsAndSave(const ntp_tiles::NTPTilesVector& most_visited_data,
                        id<GoogleLandingDataSource> favicon_fetcher,
                        NSURL* favicons_url) {
  NSMutableDictionary<NSURL*, NTPTile*>* tiles =
      [[NSMutableDictionary alloc] init];

  // If there are 0 sites to display, the for loop below will not be entered.
  // Write the updated empty list of sites to disk before returning.
  if (most_visited_data.empty()) {
    WriteToDiskIfComplete(tiles, favicons_url);
    return;
  }

  // For each site, get the favicon. If it is returned, write it to the
  // favicon tmp folder. If a fallback value is returned, update the tile
  // info. Calls WriteToDiskIfComplete after each callback execution. All the
  // sites are added first to the list so that the WriteToDiskIfComplete
  // command is not passed an incomplete list.
  for (size_t i = 0; i < most_visited_data.size(); i++) {
    const ntp_tiles::NTPTile& ntp_tile = most_visited_data[i];
    NTPTile* tile =
        [[NTPTile alloc] initWithTitle:base::SysUTF16ToNSString(ntp_tile.title)
                                   URL:net::NSURLWithGURL(ntp_tile.url)
                              position:i];
    [tiles setObject:tile forKey:tile.URL];
  }

  NSURL* tmpFaviconURL = GetTemporaryFaviconFolderPath();
  for (NTPTile* tile : [tiles objectEnumerator]) {
    const GURL& gurl = net::GURLWithNSURL(tile.URL);
    NSString* faviconFileName = GetFaviconFileName(gurl);
    NSURL* fileURL =
        [tmpFaviconURL URLByAppendingPathComponent:faviconFileName];

    void (^faviconImageBlock)(UIImage*) = ^(UIImage* favicon) {
      NSData* imageData = UIImagePNGRepresentation(favicon);

      base::OnceCallback<BOOL()> writeImage = base::BindBlockArc(^BOOL() {
        return [imageData writeToURL:fileURL atomically:YES];
      });

      base::OnceCallback<void(BOOL)> updateTile =
          base::BindBlockArc(^(BOOL imageWriteSuccess) {
            if (imageWriteSuccess) {
              // If saving the image is not possible, the best thing to do is to
              // keep the previous tiles.
              tile.faviconFetched = YES;
              tile.faviconFileName = faviconFileName;
              WriteToDiskIfComplete(tiles, favicons_url);
            }
          });

      base::PostTaskWithTraitsAndReplyWithResult(
          FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
          std::move(writeImage), std::move(updateTile));
    };

    void (^fallbackBlock)(UIColor*, UIColor*, BOOL) =
        ^(UIColor* textColor, UIColor* backgroundColor, BOOL isDefaultColor) {
          tile.faviconFetched = YES;
          tile.fallbackTextColor = textColor;
          tile.fallbackBackgroundColor = backgroundColor;
          tile.fallbackIsDefaultColor = isDefaultColor;
          tile.fallbackMonogram = base::SysUTF16ToNSString(
              favicon::GetFallbackIconText(net::GURLWithNSURL(tile.URL)));
          DCHECK(tile.fallbackMonogram && tile.fallbackTextColor &&
                 tile.fallbackBackgroundColor);
          WriteToDiskIfComplete(tiles, favicons_url);
        };

    // The cache is not used here as it is simply a way to have a synchronous
    // immediate return. In this case it is unnecessary.

    [favicon_fetcher getFaviconForURL:gurl
                                 size:48
                             useCache:NO
                        imageCallback:faviconImageBlock
                     fallbackCallback:fallbackBlock];
  }
}

void SaveMostVisitedToDisk(const ntp_tiles::NTPTilesVector& most_visited_data,
                           id<GoogleLandingDataSource> favicon_fetcher,
                           NSURL* favicons_url) {
  if (favicons_url == nil) {
    return;
  }

  base::PostTaskWithTraitsAndReply(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::BindOnce(&ClearTemporaryFaviconFolder), base::BindBlockArc(^{
        GetFaviconsAndSave(most_visited_data, favicon_fetcher, favicons_url);
      }));
}

void WriteSingleUpdatedTileToDisk(NTPTile* tile) {
  NSMutableDictionary* tiles = [ReadSavedMostVisited() mutableCopy];
  [tiles setObject:tile forKey:tile.URL];
  WriteSavedMostVisited(tiles);
}

void WriteSavedMostVisited(NSDictionary<NSURL*, NTPTile*>* most_visited_data) {
  NSData* data = [NSKeyedArchiver archivedDataWithRootObject:most_visited_data];
  NSUserDefaults* sharedDefaults = app_group::GetGroupUserDefaults();
  [sharedDefaults setObject:data forKey:app_group::kSuggestedItems];

  // TODO(crbug.com/750673): Update the widget's visibility depending on
  // availability of sites.
}

NSDictionary* ReadSavedMostVisited() {
  NSUserDefaults* sharedDefaults = app_group::GetGroupUserDefaults();

  return [NSKeyedUnarchiver
      unarchiveObjectWithData:[sharedDefaults
                                  objectForKey:app_group::kSuggestedItems]];
}

void UpdateSingleFavicon(const GURL& site_url,
                         id<GoogleLandingDataSource> favicon_fetcher,
                         NSURL* favicons_url) {
  NSDictionary* tiles = ReadSavedMostVisited();

  NSURL* siteNSURL = net::NSURLWithGURL(site_url);
  NTPTile* tile = [tiles objectForKey:siteNSURL];
  if (!tile) {
    return;
  }
  // Remove existing favicon info
  tile.fallbackTextColor = nil;
  tile.fallbackBackgroundColor = nil;
  tile.faviconFetched = NO;
  NSString* previousFaviconFileName = tile.faviconFileName;
  tile.faviconFileName = nil;

  // Fetch favicon and update saved defaults.
  NSString* faviconFileName = GetFaviconFileName(site_url);
  NSURL* fileURL = [favicons_url URLByAppendingPathComponent:faviconFileName];
  NSURL* previousFileURL =
      previousFaviconFileName
          ? [favicons_url URLByAppendingPathComponent:previousFaviconFileName]
          : nil;
  NSString* monogram =
      base::SysUTF16ToNSString(favicon::GetFallbackIconText(site_url));

  void (^faviconImageBlock)(UIImage*) = ^(UIImage* favicon) {
    tile.faviconFetched = YES;
    NSData* imageData = UIImagePNGRepresentation(favicon);

    base::OnceCallback<BOOL()> writeImage = base::BindBlockArc(^BOOL {
      [[NSFileManager defaultManager] removeItemAtURL:previousFileURL
                                                error:nil];
      return [imageData writeToURL:fileURL atomically:YES];
    });

    base::OnceCallback<void(BOOL)> updateTile =
        base::BindBlockArc(^(BOOL imageWriteSuccess) {
          if (imageWriteSuccess) {
            tile.faviconFileName = faviconFileName;
          }
          WriteSingleUpdatedTileToDisk(tile);

        });

    base::PostTaskWithTraitsAndReplyWithResult(
        FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
        std::move(writeImage), std::move(updateTile));

  };

  void (^fallbackBlock)(UIColor*, UIColor*, BOOL) =
      ^(UIColor* textColor, UIColor* backgroundColor, BOOL isDefaultColor) {
        tile.faviconFetched = YES;
        tile.fallbackTextColor = textColor;
        tile.fallbackBackgroundColor = backgroundColor;
        tile.fallbackIsDefaultColor = isDefaultColor;
        tile.fallbackMonogram = monogram;

        base::OnceCallback<void()> removeImage = base::BindBlockArc(^{
          [[NSFileManager defaultManager] removeItemAtURL:previousFileURL
                                                    error:nil];
        });

        base::PostTaskWithTraitsAndReply(
            FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
            std::move(removeImage), base::BindBlockArc(^{
              WriteSingleUpdatedTileToDisk(tile);
            }));
      };

  // The cache is not used here as it is simply a way to have a synchronous
  // immediate return. In this case it is unnecessary.
  [favicon_fetcher getFaviconForURL:site_url
                               size:48
                           useCache:NO
                      imageCallback:faviconImageBlock
                   fallbackCallback:fallbackBlock];
}
}  // namespace ntp_tile_saver
