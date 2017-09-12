// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_KEYBOARD_LOCK_PAGE_OBSERVER_H_
#define CONTENT_BROWSER_KEYBOARD_LOCK_PAGE_OBSERVER_H_

#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/web_contents_observer.h"
#include "ui/aura/keyboard_lock/observer.h"

namespace content {
class WebContents;

namespace keyboard_lock {

class PageObserver final
      public content::WebContentsObserver {
 public:
  using ObserverType = ui::aura::keyboard_lock::Observer<content::WebContents*>;

  PageObserver(content::WebContents* client,
               std::unique_ptr<ObserverType> observer);

 private:
  // PageObserver has the same lifetime as its WebContents.
  ~PageObserver();

  void DidToggleFullscreenModeForTab(bool entered_fullscreen,
                                     bool will_cause_resize) override;
  void OnWebContentsLostFocus() override;
  void OnWebContentsFocused() override;
  void WebContentsDestroyed() override;

  void TriggerObserver();

  std::unique_ptr<ObserverType> observer_;

  bool is_fullscreen_ = false;
  bool is_focused_ = false;
  content::NotificationRegistrar registrar_;
};

}  // namespace keyboard_lock
}  // namespace content

#endif  // CONTENT_BROWSER_KEYBOARD_LOCK_PAGE_OBSERVER_H_
