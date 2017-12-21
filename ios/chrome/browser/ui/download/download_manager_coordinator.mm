// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/download/download_manager_coordinator.h"

#import "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#import "ios/chrome/browser/ui/download/download_manager_view_controller.h"
#import "ios/chrome/browser/ui/presenters/contained_presenter.h"
#import "ios/chrome/browser/ui/presenters/contained_presenter_delegate.h"
#import "ios/web/public/download/download_task.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface DownloadManagerCoordinator ()<DownloadManagerViewControllerDelegate,
                                         ContainedPresenterDelegate> {
  DownloadManagerViewController* _viewController;
}
@end

@implementation DownloadManagerCoordinator

@synthesize presenter = _presenter;
@synthesize downloadTask = _downloadTask;

- (void)start {
  DCHECK(self.presenter);

  _viewController = [[DownloadManagerViewController alloc] init];
  _viewController.delegate = self;
  _viewController.fileName =
      base::SysUTF16ToNSString(_downloadTask->GetSuggestedFilename());

  self.presenter.baseViewController = self.baseViewController;
  self.presenter.presentedViewController = _viewController;
  self.presenter.delegate = self;

  [self.presenter prepareForPresentation];

  [self.presenter presentAnimated:YES];
}

- (void)stop {
  [self.presenter dismissAnimated:YES];
  _viewController = nil;
  _downloadTask = nullptr;
}

#pragma mark - DownloadManagerTabHelperDelegate

- (void)downloadManagerTabHelper:(nonnull DownloadManagerTabHelper*)tabHelper
              didRequestDownload:(nonnull web::DownloadTask*)download {
  _downloadTask = download;
  [self start];
}

#pragma mark - ContainedPresenterDelegate

- (void)containedPresenterDidDismiss:(id<ContainedPresenter>)presenter {
  DCHECK(presenter == self.presenter);
}

#pragma mark - DownloadManagerViewControllerDelegate

- (void)downloadManagerViewControllerDidClose:
    (DownloadManagerViewController*)controller {
  _downloadTask->Cancel();
  [self stop];
}

@end
