// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/download/download_manager_coordinator.h"

#include <memory>

#include "base/files/file_util.h"
#import "base/logging.h"
#import "base/mac/bind_objc_block.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "base/threading/thread_task_runner_handle.h"
#import "ios/chrome/browser/ui/download/download_manager_view_controller.h"
#import "ios/chrome/browser/ui/presenters/contained_presenter.h"
#import "ios/chrome/browser/ui/presenters/contained_presenter_delegate.h"
#import "ios/web/public/download/download_task.h"
#include "ios/web/public/web_thread.h"
#include "net/url_request/url_fetcher_response_writer.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using web::WebThread;

namespace {

//
bool GetDownloadDirectory(base::FilePath* path) {
  base::FilePath temp_dir;
  if (!GetTempDir(&temp_dir)) {
    return false;
  }

  *path = temp_dir.Append("downloads");
  return true;
}
}

@interface DownloadManagerCoordinator ()<DownloadManagerViewControllerDelegate,
                                         ContainedPresenterDelegate> {
  DownloadManagerViewController* _viewController;
  UIDocumentInteractionController* _openInController;
}
@end

@implementation DownloadManagerCoordinator

@synthesize presenter = _presenter;
@synthesize downloadTask = _downloadTask;
@synthesize animatesPresentation = _animatesPresentation;

- (void)start {
  DCHECK(self.presenter);

  _viewController = [[DownloadManagerViewController alloc] init];
  _viewController.delegate = self;
  [self updateViewController];

  self.presenter.baseViewController = self.baseViewController;
  self.presenter.presentedViewController = _viewController;
  self.presenter.delegate = self;

  [self.presenter prepareForPresentation];

  [self.presenter presentAnimated:self.animatesPresentation];
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
  self.animatesPresentation = YES;
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

- (void)downloadManagerTabHelper:(nonnull DownloadManagerTabHelper*)tabHelper
                 didHideDownload:(nonnull web::DownloadTask*)download {
  if (!_downloadTask)
    return;

  DCHECK_EQ(_downloadTask, download);
  [self stop];
}

- (void)downloadManagerTabHelper:(nonnull DownloadManagerTabHelper*)tabHelper
                 didShowDownload:(nonnull web::DownloadTask*)download {
  _downloadTask = download;
  self.animatesPresentation = NO;
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

- (void)downloadManagerViewControllerDidStartDownload:
    (DownloadManagerViewController*)controller {
  [self startDownload];
}

- (void)downloadManagerViewController:(DownloadManagerViewController*)controller
            presentOpenInMenuFromRect:(CGRect)rect {
  base::FilePath path =
      _downloadTask->GetResponseWriter()->AsFileWriter()->file_path();
  NSURL* URL = [NSURL fileURLWithPath:base::SysUTF8ToNSString(path.value())];
  _openInController =
      [UIDocumentInteractionController interactionControllerWithURL:URL];

  BOOL menuShown = [_openInController presentOpenInMenuFromRect:rect
                                                         inView:controller.view
                                                       animated:YES];
  DCHECK(menuShown);
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
      // Download Manager should dismiss the UI after download cancellation.
      NOTREACHED();
      return kDownloadManagerStateNotStarted;
  }
}

// Asynchrnonously starts download operation.
- (void)startDownload {
  // TODO: disable download button.
  base::FilePath downloadDir;
  if (!GetDownloadDirectory(&downloadDir)) {
    NOTREACHED();
    return;
  }

  base::string16 suggestedFileName = _downloadTask->GetSuggestedFilename();
  __weak DownloadManagerCoordinator* weakSelf = self;
  base::TaskTraits traits(base::MayBlock(), base::TaskPriority::USER_VISIBLE);
  base::PostTaskWithTraits(FROM_HERE, traits, base::BindBlockArc(^{
    if (!weakSelf)
      return;

    if (!base::CreateDirectory(downloadDir)) {
      NOTREACHED();
      return;
    }

    base::FilePath donwloadFilePath =
        downloadDir.Append(base::UTF16ToUTF8(suggestedFileName));

    // TODO: only start download on UI thread. net::URLFetcherFileWriter can be
    // created and initialized on background thread.
    WebThread::PostTask(WebThread::UI, FROM_HERE, base::BindBlockArc(^{
      __block auto writer = std::make_unique<net::URLFetcherFileWriter>(
          base::CreateSequencedTaskRunnerWithTraits(
              {base::MayBlock(), base::TaskPriority::BACKGROUND}),
          donwloadFilePath);

      writer->Initialize(base::BindBlockArc(^(int error) {
        DCHECK(!error);
        weakSelf.downloadTask->Start(std::move(writer));
      }));
    }));
  }));
}

@end
