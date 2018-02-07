// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_CONTENT_DOWNLOAD_ITEM_DELEGATE_H_
#define CONTENT_PUBLIC_BROWSER_CONTENT_DOWNLOAD_ITEM_DELEGATE_H_

#include "components/download/public/common/download_item_delegate.h"
#include "content/common/content_export.h"

namespace download {
class DownloadItem;
}

namespace content {

class BrowserContext;
class WebContents;

// Delegate to provide content layer information about a DownloadItem to
// embedders.
class CONTENT_EXPORT ContentDownloadItemDelegate
    : public download::DownloadItemDelegate {
 public:
  // Helper method to get the BrowserContext from a DownloadItem.
  static BrowserContext* GetBrowserContext(
      const download::DownloadItem* downloadItem);

  // Helper method to get the WebContents from a DownloadItem.
  static WebContents* GetWebContents(
      const download::DownloadItem* downloadItem);

  // BrowserContext that indirectly owns this download. Always valid.
  virtual BrowserContext* GetBrowserContext() const = 0;

  // WebContents associated with the download. Returns nullptr if the
  // WebContents is unknown or if the download was not performed on behalf of a
  // renderer.
  virtual WebContents* GetWebContents() const = 0;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_CONTENT_DOWNLOAD_ITEM_DELEGATE_H_
