// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_COMMON_H_
#define CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_COMMON_H_

#include "base/callback_forward.h"
#include "content/public/browser/web_contents.h"

namespace content {
namespace service_worker {

extern const char kNoDocumentURLErrorMessage[];
extern const char kShutdownErrorMessage[];
extern const char kUserDeniedPermissionMessage[];
extern const char kInvalidStateErrorMessage[];
extern const char kBadMessageImproperOrigins[];

WebContents* GetWebContents(int render_process_id, int render_frame_id);

void RunSoon(const base::Closure& closure);  // FIXME: includes

}  // namespace service_worker
}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_COMMON_H_
