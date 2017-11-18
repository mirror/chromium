// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_CONTENT_ARC_CONTEXT_H_
#define COMPONENTS_ARC_CONTENT_ARC_CONTEXT_H_

namespace content {
class BrowserContext;
}  // namespace content

namespace arc {

class ArcContext {
 public:
  explicit ArcContext(content::BrowserContext* context);
  ~ArcContext();

  static ArcContext* FromBrowserContext(content::BrowserContext* context);

  content::BrowserContext* browser_context() { return browser_context_; }

 private:
  content::BrowserContext* const browser_context_;
};

}  // namespace arc

#endif  // COMPONENTS_ARC_CONTENT_ARC_CONTEXT_H_
