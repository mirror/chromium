// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOWNLOAD_CONTENT_DOWNLOAD_ITEM_DELEGATE_IMPL_H_
#define CONTENT_BROWSER_DOWNLOAD_CONTENT_DOWNLOAD_ITEM_DELEGATE_IMPL_H_

#include "content/common/content_export.h"
#include "content/public/browser/content_download_item_delegate.h"

namespace content {
class BrowserContext;

class CONTENT_EXPORT ContentDownloadItemDelegateImpl
    : public ContentDownloadItemDelegate {
 public:
  ContentDownloadItemDelegateImpl(BrowserContext* browser_context,
                                  WebContents* web_contents);

  ~ContentDownloadItemDelegateImpl() override;

  // ContentDownloadItemDelegate:
  BrowserContext* GetBrowserContext() const override;
  WebContents* GetWebContents() const override;

 private:
  BrowserContext* browser_context_;
  int render_process_id_;
  int render_view_id_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOWNLOAD_CONTENT_DOWNLOAD_ITEM_DELEGATE_IMPL_H_
