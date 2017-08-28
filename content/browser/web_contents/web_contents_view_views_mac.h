// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_VIEW_VIEWS_MAC_H_
#define CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_VIEW_VIEWS_MAC_H_

#import "content/browser/web_contents/web_contents_view_mac.h"

CONTENT_EXPORT
@interface WebContentsViewViewsCocoa : WebContentsViewCocoa {
}

@end

namespace content {

// MacViews-specific implementation of the WebContentsView.
class WebContentsViewViewsMac : public WebContentsViewMac {
 public:
  WebContentsViewViewsMac(WebContentsImpl* web_contents,
                          WebContentsViewDelegate* delegate);
  ~WebContentsViewViewsMac() override;

  // WebContentsViewMac:
  void CreateView(const gfx::Size& initial_size,
                  gfx::NativeView context) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(WebContentsViewViewsMac);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_VIEW_VIEWS_MAC_H_
