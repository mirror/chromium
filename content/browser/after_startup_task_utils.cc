// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/after_startup_task_utils.h"

#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_client.h"

namespace content {

void SetBrowserStartupIsCompleteForTesting() {
  GetContentClient()->browser()->SetBrowserStartupIsCompleteForTesting();
}

}  // namespace content
