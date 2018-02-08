// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/download/content_download_item_delegate_impl.h"

#include "components/download/public/common/download_item.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"

namespace content {

// static
BrowserContext* ContentDownloadItemDelegate::GetBrowserContext(
    const download::DownloadItem* download_item) {
  download::DownloadItemDelegate* delegate = download_item->GetDelegate();
  if (!delegate)
    return nullptr;
  return static_cast<ContentDownloadItemDelegate*>(delegate)
      ->GetBrowserContext();
}

// static
WebContents* ContentDownloadItemDelegate::GetWebContents(
    const download::DownloadItem* download_item) {
  download::DownloadItemDelegate* delegate = download_item->GetDelegate();
  if (!delegate)
    return nullptr;
  return static_cast<ContentDownloadItemDelegate*>(delegate)->GetWebContents();
}

ContentDownloadItemDelegateImpl::ContentDownloadItemDelegateImpl(
    BrowserContext* browser_context,
    WebContents* web_contents)
    : browser_context_(browser_context) {
  render_process_id_ = 0;
  render_view_id_ = 0;
  if (web_contents) {
    RenderViewHost* render_view_host = web_contents->GetRenderViewHost();
    render_process_id_ = render_view_host->GetProcess()->GetID();
    render_view_id_ = render_view_host->GetRoutingID();
  }
}

ContentDownloadItemDelegateImpl::~ContentDownloadItemDelegateImpl() = default;

BrowserContext* ContentDownloadItemDelegateImpl::GetBrowserContext() const {
  return browser_context_;
}

WebContents* ContentDownloadItemDelegateImpl::GetWebContents() const {
  RenderViewHost* render_view_host =
      RenderViewHost::FromID(render_process_id_, render_view_id_);
  return WebContents::FromRenderViewHost(render_view_host);
}

}  // namespace content
