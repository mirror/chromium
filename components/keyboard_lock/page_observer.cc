// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/keyboard_lock/page_observer.h"

#include "base/callback.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "chrome/browser/chrome_notification_types.h"
/* cycle dependency
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
*/
#include "components/keyboard_lock/keyboard_lock_host.h"
#include "components/keyboard_lock/key_hook_activator.h"
#include "components/keyboard_lock/key_hook_activator_collection.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/web_contents.h"

namespace keyboard_lock {

// static
void PageObserver::Observe(content::WebContents* web_contents,
                           KeyHookActivatorCollection* collection) {
  new PageObserver(web_contents, collection);
}

PageObserver::PageObserver(
    content::WebContents* contents,
    KeyHookActivatorCollection* collection)
    : WebContentsObserver(contents),
      collection_(collection) {
  DCHECK(web_contents());
  LOG(ERROR) << "PageObserver observes on thread " << base::PlatformThread::CurrentId();
  // TODO(zijiehe): Cannot receive this notification.
  is_fullscreen_ = true;
  registrar_.Add(this,
                 chrome::NOTIFICATION_FULLSCREEN_CHANGED,
                 content::NotificationService::AllSources());
  /* cycle dependency
                 content::Source<FullscreenController>(
                     chrome::FindBrowserWithWebContents(web_contents())
                         ->exclusive_access_manager()
                         ->fullscreen_controller()));
  */
}

PageObserver::~PageObserver() {
  collection_->Erase(web_contents());
}

void PageObserver::Observe(int type,
                           const content::NotificationSource& source,
                           const content::NotificationDetails& details) {
  DCHECK_EQ(chrome::NOTIFICATION_FULLSCREEN_CHANGED, type);
  is_fullscreen_ = *(content::Details<bool>(details).ptr());
  LOG(ERROR) << "Received fullscreen notification: "
             << (is_fullscreen_ ? "True" : "False");
  TriggerActivator();
}

void PageObserver::OnWebContentsLostFocus(content::RenderWidgetHost* render_widget_host) {
  is_focused_ = false;
  LOG(ERROR) << "Received OnWebContentsLostFocus";
  TriggerActivator();
}

void PageObserver::OnWebContentsFocused(content::RenderWidgetHost* render_widget_host) {
  is_focused_ = true;
  LOG(ERROR) << "Received OnWebContentsFocused";
  TriggerActivator();
}

void PageObserver::WebContentsDestroyed() {
  LOG(ERROR) << "Received WebContentsDestroyed";
  KeyHookActivator* activator = collection_->Find(web_contents());
  if (activator) {
    activator->Deactivate(base::Callback<void(bool)>());
  }
  delete this;
}

void PageObserver::TriggerActivator() {
  KeyHookActivator* activator = collection_->Find(web_contents());
  if (activator == nullptr) {
    LOG(ERROR) << "No KeyHookActivator found for WebContents "
               << web_contents();
    // The WebContents may be called CancelKeyboardLock(). This observer is not
    // necessary anymore.
    // "delete this" immediately stops all following callbacks.
    delete this;
    return;
  }

  if (is_fullscreen_ && is_focused_) {
    LOG(ERROR) << "Going to Activate";
    activator->Activate(base::Callback<void(bool)>());
  } else {
    LOG(ERROR) << "Going to Deactivate";
    activator->Deactivate(base::Callback<void(bool)>());
  }
}

}  // namespace keyboard_lock
