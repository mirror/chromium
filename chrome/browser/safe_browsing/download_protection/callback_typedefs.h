// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SAFE_BROWSING_DOWNLOAD_PROTECTION_CALLBACK_TYPEDEFS_H_
#define CHROME_BROWSER_SAFE_BROWSING_DOWNLOAD_PROTECTION_CALLBACK_TYPEDEFS_H_

#include "chrome/browser/safe_browsing/download_protection/download_check_enums.h"
#include "content/public/browser/download_item.h"

namespace safe_browsing {

// Callback type which is invoked once the download request is done.
typedef base::Callback<void(DownloadCheckResult)> CheckDownloadCallback;

// A type of callback run on the main thread when a ClientDownloadRequest has
// been formed for a download, or when one has not been formed for a supported
// download.
typedef base::Callback<void(content::DownloadItem*,
                            const ClientDownloadRequest*)>
    ClientDownloadRequestCallback;

// A list of ClientDownloadRequest callbacks.
typedef base::CallbackList<void(content::DownloadItem*,
                                const ClientDownloadRequest*)>
    ClientDownloadRequestCallbackList;

// A subscription to a registered ClientDownloadRequest callback.
typedef std::unique_ptr<ClientDownloadRequestCallbackList::Subscription>
    ClientDownloadRequestSubscription;

// A type of callback run on the main thread when a PPAPI
// ClientDownloadRequest has been formed for a download.
typedef base::Callback<void(const ClientDownloadRequest*)>
    PPAPIDownloadRequestCallback;

// A list of PPAPI ClientDownloadRequest callbacks.
typedef base::CallbackList<void(const ClientDownloadRequest*)>
    PPAPIDownloadRequestCallbackList;

// A subscription to a registered PPAPI ClientDownloadRequest callback.
typedef std::unique_ptr<PPAPIDownloadRequestCallbackList::Subscription>
    PPAPIDownloadRequestSubscription;

}  // namespace safe_browsing

#endif  // CHROME_BROWSER_SAFE_BROWSING_DOWNLOAD_PROTECTION_CALLBACK_TYPEDEFS_H_
