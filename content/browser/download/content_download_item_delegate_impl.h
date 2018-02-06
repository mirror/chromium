// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOWNLOAD_CONTENT_DOWNLOAD_ITEM_DELEGATE_IMPL_H_
#define CONTENT_BROWSER_DOWNLOAD_CONTENT_DOWNLOAD_ITEM_DELEGATE_IMPL_H_

#include "content/common/content_export.h"
#include "content/public/browser/content_download_item_delegate.h"
#include "content/public/browser/resource_request_info.h"

namespace content {
class BrowserContext;

// Delegate for operations that a DownloadItemImpl can't do for itself.
// The base implementation of this class does nothing (returning false
// on predicates) so interfaces not of interest to a derived class may
// be left unimplemented.
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
  content::ResourceRequestInfo::WebContentsGetter web_contents_getter_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOWNLOAD_CONTENT_DOWNLOAD_ITEM_DELEGATE_IMPL_H_
