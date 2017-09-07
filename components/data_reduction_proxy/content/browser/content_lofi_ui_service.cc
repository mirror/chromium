// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/content/browser/content_lofi_ui_service.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/browser/web_contents.h"
#include "net/url_request/url_request.h"

namespace data_reduction_proxy {

ContentLoFiUIService::ContentLoFiUIService(
    const scoped_refptr<base::SingleThreadTaskRunner>& ui_task_runner,
    const OnPreviewsShownCallback& notify_previews_shown_callback)
    : ui_task_runner_(ui_task_runner),
      on_previews_shown_callback_(notify_previews_shown_callback) {
  DCHECK(!on_previews_shown_callback_.is_null());
}

ContentLoFiUIService::~ContentLoFiUIService() {}

void ContentLoFiUIService::OnPreviewsShown(const net::URLRequest& request,
                                           previews::PreviewsType type,
                                           const GURL& non_preview_url) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  int render_process_id = -1;
  int render_frame_id = -1;
  if (content::ResourceRequestInfo::GetRenderFrameForRequest(
          &request, &render_process_id, &render_frame_id)) {
    ui_task_runner_->PostTask(
        FROM_HERE, base::Bind(&ContentLoFiUIService::OnPreviewsShownOnUIThread,
                              base::Unretained(this), render_process_id,
                              render_frame_id, type, non_preview_url));
  }
}

void ContentLoFiUIService::OnPreviewsShownOnUIThread(
    int render_process_id,
    int render_frame_id,
    previews::PreviewsType type,
    const GURL& non_preview_url) {
  DCHECK(ui_task_runner_->BelongsToCurrentThread());
  content::RenderFrameHost* frame =
      content::RenderFrameHost::FromID(render_process_id, render_frame_id);
  if (frame) {
    DCHECK(!on_previews_shown_callback_.is_null());
    content::WebContents* web_contents =
        content::WebContents::FromRenderFrameHost(frame);
    on_previews_shown_callback_.Run(web_contents, type, non_preview_url);
  }
}

}  // namespace data_reduction_proxy
