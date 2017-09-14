// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_KEYBOARD_LOCK_PAGE_OBSERVER_H_
#define CONTENT_BROWSER_KEYBOARD_LOCK_PAGE_OBSERVER_H_

#include "content/public/browser/web_contents_observer.h"
#include "ui/aura/keyboard_lock/observer.h"

namespace content {
class NavigationHandle;
class RenderWidgetHost;
class WebContents;

namespace keyboard_lock {

class PageObserver final
    : public content::WebContentsObserver {
 public:
  using ObserverType = ui::aura::keyboard_lock::Observer<content::WebContents*>;

  PageObserver(content::WebContents* client,
               std::unique_ptr<ObserverType> observer);

 private:
  // PageObserver has the same lifetime as its WebContents.
  ~PageObserver() override;

  void DidToggleFullscreenModeForTab(bool entered_fullscreen,
                                     bool will_cause_resize) override;
  void DidStartNavigation(NavigationHandle* navigation_handle) override;
  void OnWebContentsLostFocus(RenderWidgetHost* render_widget_host) override;
  void OnWebContentsFocused(RenderWidgetHost* render_widget_host) override;
  void WebContentsDestroyed() override;

  void TriggerObserver();

  std::unique_ptr<ObserverType> observer_;

  bool is_fullscreen_ = false;
  bool is_focused_ = false;
};

}  // namespace keyboard_lock
}  // namespace content

#endif  // CONTENT_BROWSER_KEYBOARD_LOCK_PAGE_OBSERVER_H_
