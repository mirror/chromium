// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/download/download_manager_coordinator.h"

#include <memory>

#import "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#import "ios/chrome/browser/ui/download/download_manager_view_controller.h"
#import "ios/chrome/browser/ui/presenters/contained_presenter.h"
#import "ios/chrome/browser/ui/presenters/contained_presenter_delegate.h"
#import "ios/web/public/download/download_task.h"
#include "net/url_request/url_fetcher_response_writer.h"

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
  [self updateViewController];

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
  // TODO: (check if web state is visible).
  _downloadTask = download;
  [self start];
}

- (void)downloadManagerTabHelper:(nonnull DownloadManagerTabHelper*)tabHelper
               didUpdateDownload:(nonnull web::DownloadTask*)download {
  if (_downloadTask != download) {
    // Download for background Tab has been updated.
    return;
  }

  [self updateViewController];
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

- (void)downloadManagerViewControllerDidStartDownload:
    (DownloadManagerViewController*)controller {
  // TODO: use URLFetcherFileWriter.
  _downloadTask->Start(std::make_unique<net::URLFetcherStringWriter>());
}

- (void)downloadManagerViewController:(DownloadManagerViewController*)controller
            presentOpenInMenuFromRect:(CGRect)rect {
  NOTREACHED();
}

#pragma mark - Private

// Updates presented view controller with web::DownloadTask data.
- (void)updateViewController {
  _viewController.state = [self downloadManagerState];
  _viewController.expectedFileSize = _downloadTask->GetTotalBytes();
  _viewController.fileName =
      base::SysUTF16ToNSString(_downloadTask->GetSuggestedFilename());
}

// Returns DownloadManagerState for the current download task.
- (DownloadManagerState)downloadManagerState {
  switch (_downloadTask->GetState()) {
    case web::DownloadTask::State::kNotStarted:
      return kDownloadManagerStateNotStarted;
    case web::DownloadTask::State::kInProgress:
      return kDownloadManagerStateInProgress;
    case web::DownloadTask::State::kComplete:
      return _downloadTask->GetErrorCode() ? kDownloadManagerStateFailed
                                           : kDownloadManagerStateSuceeded;
    case web::DownloadTask::State::kCancelled:
      // Download Manager should dismiss the UI after donwload cancellation.
      NOTREACHED();
      return kDownloadManagerStateNotStarted;
  }
}

@end
