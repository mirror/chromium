// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_NTP_NTP_TILE_H_
#define IOS_CHROME_BROWSER_UI_NTP_NTP_TILE_H_

#import <UIKit/UIKit.h>

@interface NTPTile : NSObject<NSCoding>

@property(readonly, atomic) NSString* title;
@property(readonly, atomic) NSURL* URL;
@property(strong, atomic) NSString* faviconPath;
@property(strong, atomic) UIColor* fallbackTextColor;
@property(strong, atomic) UIColor* fallbackBackgroundColor;
@property(assign, atomic) BOOL fallbackIsDefaultColor;

- (instancetype)initWithTitle:(NSString*)title URL:(NSURL*)URL;
- (instancetype)initWithTitle:(NSString*)title
                          URL:(NSURL*)URL
                  faviconPath:(NSString*)faviconPath
            fallbackTextColor:(UIColor*)fallbackTextColor
      fallbackBackgroundColor:(UIColor*)fallbackTextColor
       fallbackIsDefaultColor:(BOOL)fallbackIsDefaultColor;
@end

#endif  // IOS_CHROME_BROWSER_UI_NTP_NTP_TILE_H_
