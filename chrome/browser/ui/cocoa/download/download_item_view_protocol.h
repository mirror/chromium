// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <AppKit/AppKit.h>

#include "base/files/file_path.h"

// TODO(sdy): This shouldn't know about DownloadItemController.
@class DownloadItemController;
class DownloadItemModel;

@protocol DownloadItemView
@property(assign, nonatomic) DownloadItemController* controller;

// TODO(sdy): A delegate would be better once the pre-MD path is gone.
@property(assign) id target;
@property SEL action;

- (void)setStateFromDownload:(DownloadItemModel*)downloadModel;

- (void)setImage:(NSImage*)image;

@optional
@property(readonly) CGFloat preferredWidth;
@end
