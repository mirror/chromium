// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_KEYBOARD_LOCK_PAGE_OBSERVER_H_
#define COMPONENTS_KEYBOARD_LOCK_PAGE_OBSERVER_H_

#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class NotificationDetails;
class NotificationSource;
class WebContents;
}  // namespace content

namespace keyboard_lock {

class KeyHookActivator;
class KeyHookActivatorCollection;

// A PageObserver instance keeps a reference of a KeyHookActivator (usually a
// KeyHookActivatorThreadProxy wrapped KeyHookStateKeeper instance) and a
// WebContents. Once an interesting event, e.g. page taking or losing focus,
// entering or exiting fullscreen, PageObserver calls
// KeyHookActivator::Activate() or KeyHookActivator::Deactivate() functions.
// A PageObserver instance has the same lifetime with its WebContents, it
// deletes itself once WebContentsObserver::WebContentsDestroyed() is called.
// This class must be instantiated in UI thread to ensure correct signals can be
// received.
class PageObserver final
    : public content::NotificationObserver,
      public content::WebContentsObserver {
 public:
  // |collection| must outlive current instance.
  static void Observe(content::WebContents* web_contents,
                      KeyHookActivatorCollection* collection);

 private:
  PageObserver(content::WebContents* web_contents,
               KeyHookActivatorCollection* collection);
  ~PageObserver() override;

  // NotificationObserver implementations
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // WebContentsObserver implementations
  void OnWebContentsLostFocus(content::RenderWidgetHost* render_widget_host) override;
  void OnWebContentsFocused(content::RenderWidgetHost* render_widget_host) override;
  void WebContentsDestroyed() override;

  void TriggerActivator();

  bool is_fullscreen_ = false;
  bool is_focused_ = false;
  content::NotificationRegistrar registrar_;
  KeyHookActivatorCollection* const collection_;
};

}  // namespace keyboard_lock

#endif  // COMPONENTS_KEYBOARD_LOCK_PAGE_OBSERVER_H_
