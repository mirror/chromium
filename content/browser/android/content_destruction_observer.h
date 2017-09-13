// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_CONTENT_DESTRUCTION_OBSERVER_H
#define CONTENT_BROWSER_ANDROID_CONTENT_DESTRUCTION_OBSERVER_H

#include "content/browser/renderer_host/render_widget_host_view_android.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {

// A light-weight class used to notify RenderWidgetHostViewAndroid of the
// event that WebContents (and therefore ContentViewCore) it was linked
// to got destroyed. There will be as many ContentDestructionObserver
// instances as RWHVA for a certain WebContents since they all need the
// notification. This class was devised so that RWHVA doesn't have to
// observe ContentViewCore destruction directly, which is a layering
// violation the access class hierarchy.
//
// It also observes RWHVA which, if destroyed before WebContents,
// leaves nothing for this class to do. Then it destroys itself immediately.
//
// The class owns itself, and destroys itself as soon as it finishes its
// job of sending the notification, or die a precocious death in the case
// described above.
class ContentDestructionObserver final
    : public WebContentsObserver,
      public RenderWidgetHostViewAndroid::DestructionObserver {
 public:
  static void Create(WebContents* web_contents,
                     RenderWidgetHostViewAndroid* rwhva);

  void RenderViewHostChanged(RenderViewHost* old_host,
                             RenderViewHost* new_host) override;
  void WebContentsDestroyed() override;
  void RenderWidgetHostViewDestroyed(
      RenderWidgetHostViewAndroid* rwhva) override;
  ~ContentDestructionObserver() override;

 private:
  ContentDestructionObserver(WebContents* web_contents,
                             RenderWidgetHostViewAndroid* rwhva);
  RenderWidgetHostViewAndroid* const rwhva_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_ANDROID_CONTENT_DESTRUCTION_OBSERVER_H
