// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_DOWNLOAD_DOWNLOAD_MANAGER_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_DOWNLOAD_DOWNLOAD_MANAGER_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

@class DownloadManagerViewController;

@protocol DownloadManagerViewControllerDelegate<NSObject>
@optional
// Called when close button was tapped. Delegate may dismiss presentation.
- (void)downloadManagerViewControllerDidClose:
    (DownloadManagerViewController*)controller;
@end

//
@interface DownloadManagerViewController : UIViewController

@property(nonatomic, weak) id<DownloadManagerViewControllerDelegate> delegate;

// Name of the file being downloaded.
@property(nonatomic) NSString* fileName;

// Button to dismiss the download toolbar.
@property(nonatomic, readonly) UIButton* closeButton;

// Label that displays the name of the file being downloaded and its size.
@property(nonatomic, readonly) UILabel* statusLabel;

@end

#endif  // IOS_CHROME_BROWSER_UI_DOWNLOAD_DOWNLOAD_MANAGER_VIEW_CONTROLLER_H_
