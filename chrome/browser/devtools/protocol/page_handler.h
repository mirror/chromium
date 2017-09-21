// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DEVTOOLS_PROTOCOL_PAGE_HANDLER_H_
#define CHROME_BROWSER_DEVTOOLS_PROTOCOL_PAGE_HANDLER_H_

#include "chrome/browser/devtools/protocol/forward.h"
#include "chrome/browser/devtools/protocol/page.h"

namespace content {
class WebContents;
}

class PageHandler : public protocol::Page::Backend {
 public:
  PageHandler(content::WebContents* web_contents,
              protocol::UberDispatcher* dispatcher);
  ~PageHandler() override;

  // Page::Backend:
  protocol::Response Enable() override;
  protocol::Response Disable() override;
  protocol::Response SetAdBlockingEnabled(bool enabled) override;

 private:
  void ToggleAdBlocking(bool enabled);

  content::WebContents* const web_contents_;
  bool enabled_ = false;

  DISALLOW_COPY_AND_ASSIGN(PageHandler);
};

#endif  // CHROME_BROWSER_DEVTOOLS_PROTOCOL_PAGE_HANDLER_H_
