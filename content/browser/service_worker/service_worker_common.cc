// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_common.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace content {
namespace service_worker {

const char kNoDocumentURLErrorMessage[] =
    "No URL is associated with the caller's document.";
const char kShutdownErrorMessage[] = "The Service Worker system has shutdown.";
const char kUserDeniedPermissionMessage[] =
    "The user denied permission to use Service Worker.";
const char kInvalidStateErrorMessage[] = "The object is in an invalid state.";
const char kBadMessageImproperOrigins[] =
    "Origins are not matching, or some cannot access service worker.";

WebContents* GetWebContents(int render_process_id, int render_frame_id) {
  RenderFrameHost* rfh =
      RenderFrameHost::FromID(render_process_id, render_frame_id);
  return WebContents::FromRenderFrameHost(rfh);
}

void RunSoon(const base::Closure& closure) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, closure);
}

}  // namespace service_worker
}  // namespace content
