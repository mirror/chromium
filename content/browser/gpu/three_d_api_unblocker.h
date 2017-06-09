// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_GPU_THREE_D_API_UNBLOCKER_H_
#define CONTENT_BROWSER_GPU_THREE_D_API_UNBLOCKER_H_

#include "base/macros.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {

class ThreeDAPIUnblocker : public WebContentsObserver {
 public:
  explicit ThreeDAPIUnblocker(WebContents* web_contents);

 private:
  // WebContentsObserver overrides.
  void ReadyToCommitNavigation(NavigationHandle* navigation_handle) override;
};

}  // namespace content

#endif  // CONTENT_BROWSER_GPU_THREE_D_API_UNBLOCKER_H_
