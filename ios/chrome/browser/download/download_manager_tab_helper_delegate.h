// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_MANAGER_TAB_HELPER_DELEGATE_H_
#define IOS_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_MANAGER_TAB_HELPER_DELEGATE_H_

#import <Foundation/Foundation.h>

class DownloadManagerTabHelper;
namespace web {
class DownloadTask;
}  // namespace web

class DownloadManagerTabHelperDelegateTabHelper;

// Delegate for PassKitTabHelper class.
@protocol DownloadManagerTabHelperDelegate<NSObject>

// Called to present Download Manager UI.
- (void)downloadManagerTabHelper:(nonnull DownloadManagerTabHelper*)tabHelper
              didRequestDownload:(nonnull web::DownloadTask*)download;

// Called to update download manager UI.
- (void)downloadManagerTabHelper:(nonnull DownloadManagerTabHelper*)tabHelper
               didUpdateDownload:(nonnull web::DownloadTask*)download;

// Called to hide Download Manager UI for the given task.
- (void)downloadManagerTabHelper:(nonnull DownloadManagerTabHelper*)tabHelper
                 didHideDownload:(nonnull web::DownloadTask*)download;

// Called to show Download Manager UI for the given task.
- (void)downloadManagerTabHelper:(nonnull DownloadManagerTabHelper*)tabHelper
                 didShowDownload:(nonnull web::DownloadTask*)download;

@end

#endif  // IOS_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_MANAGER_TAB_HELPER_DELEGATE_H_
