// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/tab_contents/debug_urls.h"

#include "content/browser/gpu/gpu_process_host_ui_shim.h"
#include "content/public/common/url_constants.h"
#include "googleurl/src/gurl.h"

namespace content {

bool HandleDebugURL(const GURL& url, content::PageTransition transition) {
  content::PageTransition base_transition =
      content::PageTransitionStripQualifier(transition);
  if (base_transition != content::PAGE_TRANSITION_TYPED)
    return false;

  if (url.host() == chrome::kChromeUIBrowserCrashHost) {
    // Induce an intentional crash in the browser process.
    CHECK(false);
    return true;
  }

  if (url == GURL(chrome::kChromeUIGpuCleanURL)) {
    GpuProcessHostUIShim* shim = GpuProcessHostUIShim::GetOneInstance();
    if (shim)
      shim->SimulateRemoveAllContext();
    return true;
  }

  if (url == GURL(chrome::kChromeUIGpuCrashURL)) {
    GpuProcessHostUIShim* shim = GpuProcessHostUIShim::GetOneInstance();
    if (shim)
      shim->SimulateCrash();
    return true;
  }

  if (url == GURL(chrome::kChromeUIGpuHangURL)) {
    GpuProcessHostUIShim* shim = GpuProcessHostUIShim::GetOneInstance();
    if (shim)
      shim->SimulateHang();
    return true;
  }

  return false;
}

}  // namespace content
