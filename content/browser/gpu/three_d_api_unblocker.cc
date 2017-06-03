// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/gpu/three_d_api_unblocker.h"

#include "content/browser/gpu/gpu_data_manager_impl.h"
#include "content/public/browser/navigation_handle.h"

namespace content {

ThreeDAPIUnblocker::ThreeDAPIUnblocker(WebContents* web_contents)
    : WebContentsObserver(web_contents) {}

void ThreeDAPIUnblocker::ReadyToCommitNavigation(
    NavigationHandle* navigation_handle) {
  // Note: attempted to override DidStartNavigation to do this check,
  // but the request hadn't started by that point. Want to do this as
  // early as possible to avoid race conditions with pages attempting to
  // access WebGL early on, which is why we're not overriding
  // DidFinishNavigation.

  // If any domains are blocked from accessing 3D APIs because they may
  // have caused the GPU to reset recently, unblock them here if the
  // user initiated this navigation. This implies that the user was
  // involved in the decision to navigate, so there's no concern about
  // denial-of-service issues.

  // TODO(kbr): remove this logging. It demonstrates that either with or
  // without --enable-browser-side-navigation, all navigations that were
  // initiated from the browser (reload button, reload menu option,
  // pressing return in the Omnibox) claim that the navigation doesn't
  // have a user gesture -- at least on macOS. The only way to get a
  // user gesture seems to be to type "window.location.reload()" into
  // DevTools' JavaScript console.
  //
  // Additionally, a top-level setTimeout which performs a
  // window.location.reload() is claimed to have a user gesture, which
  // seems wrong.
  LOG(ERROR) << "ReadyToCommitNavigation for URL: "
             << navigation_handle->GetURL()
             << " HasUserGesture: " << navigation_handle->HasUserGesture();

  if (navigation_handle->HasUserGesture()) {
    GpuDataManagerImpl::GetInstance()->UnblockDomainFrom3DAPIs(
        navigation_handle->GetURL());
  }
}

}  // namespace content
