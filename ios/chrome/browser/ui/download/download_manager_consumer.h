// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_DOWNLOAD_DOWNLOAD_MANAGER_CONSUMER_H_
#define IOS_CHROME_BROWSER_UI_DOWNLOAD_DOWNLOAD_MANAGER_CONSUMER_H_

#import <Foundation/Foundation.h>

typedef NS_ENUM(NSInteger, DownloadManagerState) {
  // Download has not started yet.
  kDownloadManagerStateNotStarted = 0,
  // Download is actively progressing.
  kDownloadManagerStateInProgress,
  // Download is completely finished without errors.
  kDownloadManagerStateSuceeded,
  // Download has failed with an error.
  kDownloadManagerStateFailed,
};

// Consumer for the download manager mediator.
@protocol DownloadManagerConsumer

// Name of the file being downloaded.
@property(nonatomic, copy) NSString* fileName;

// The received size of the file being downloaded in bytes.
@property(nonatomic) int64_t countOfBytesReceived;

// The expected size of the file being downloaded in bytes.
@property(nonatomic) int64_t countOfBytesExpectedToReceive;

// State of the download task. Default is kDownloadManagerStateNotStarted.
@property(nonatomic) DownloadManagerState state;

@end

#endif  // IOS_CHROME_BROWSER_UI_DOWNLOAD_DOWNLOAD_MANAGER_CONSUMER_H_
