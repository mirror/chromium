// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_IMPL_OBSERVER_H_
#define CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_IMPL_OBSERVER_H_

#include "base/macros.h"
#include "content/common/content_export.h"

namespace content {

class WebContentsImpl;

// Observer class for WebContentsImpl, for things that we don't need to expose
/// outside of content/ .
class CONTENT_EXPORT WebContentsImplObserver {
 public:
  // Called when persistent video mode is requested / un-requested.
  virtual void PersistentVideoRequested(bool want_persistent_video) {}

 protected:
  WebContentsImplObserver(WebContentsImpl* impl);
  virtual ~WebContentsImplObserver();

 private:
  WebContentsImpl* impl_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsImplObserver);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_IMPL_OBSERVER_H_
