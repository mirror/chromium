// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_DOWNLOAD_DOWNLOAD_MANAGER_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_DOWNLOAD_DOWNLOAD_MANAGER_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

@class DownloadManagerViewController;

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

@protocol DownloadManagerViewControllerDelegate<NSObject>
@optional
// Called when close button was tapped. Delegate may dismiss presentation.
- (void)downloadManagerViewControllerDidClose:
    (DownloadManagerViewController*)controller;

// Called when Download or Restart button was tapped. Delegate should start the
// download.
- (void)downloadManagerViewControllerDidStartDownload:
    (DownloadManagerViewController*)controller;

// Called when "download"Open In.." button was tapped. Delegate should present
// OpenIn system UI.
- (void)downloadManagerViewController:(DownloadManagerViewController*)controller
            presentOpenInMenuFromRect:(CGRect)rect;

@end

// Presents UI for a single download task.
@interface DownloadManagerViewController : UIViewController

@property(nonatomic, weak) id<DownloadManagerViewControllerDelegate> delegate;

// Name of the file being downloaded.
@property(nonatomic) NSString* fileName;

// The expected size of the file being downloaded in bytes.
@property(nonatomic, readwrite) long long expectedFileSize;

// State of the download task. Default is kDownloadManagerStateNotStarted.
@property(nonatomic) DownloadManagerState state;

// Button to dismiss the download toolbar.
@property(nonatomic, readonly) UIButton* closeButton;

// Label that displays the name of the file being downloaded.
@property(nonatomic, readonly) UIButton* fileNameLabel;

// Label that describes the current download status ("Download", "Downloading",
// "Downloaded", "Couldn't Download").
@property(nonatomic, readonly) UIButton* statusLabel;

// Label the displays expected file size if the download has not started,
// downloaded data size and expected file size if the download is in progress,
// and "Downloaded" if the download is complete.
@property(nonatomic, readonly) UIButton* progressLabel;

// Button to open downloaded file.
@property(nonatomic, readonly) UIButton* openInButton;

// Button to restart failed download.
@property(nonatomic, readonly) UIButton* restartButton;

@end

#endif  // IOS_CHROME_BROWSER_UI_DOWNLOAD_DOWNLOAD_MANAGER_VIEW_CONTROLLER_H_
