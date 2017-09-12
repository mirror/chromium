// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/keyboard_lock/page_observer.h"

#include <utility>

#include "base/checks.h"
#include "content/public/browser/notification_service.h"

namespace content {
namespace keyboard_lock {

PageObserver::PageObserver(content::WebContents* contents,
                           std::unique_ptr<ObserverType> observer)
    : WebContentsObserver(contents),
      observer_(std::move(observer)) {
  DCHECK(observer_);
  is_fullscreen_ = contents->IsFullscreenForCurrentTab();
  is_focused_ = (contents->GetFocusedFrame() != nullptr);
}

PageObserver::~PageObserver() {
  observer_->Erase();
}

void PageObserver::DidToggleFullscreenModeForTab(bool entered_fullscreen,
                                                 bool will_cause_resize) {
  is_fullscreen_ = entered_fullscreen;
  TriggerObserver();
}

void PageObserver::OnWebContentsLostFocus() {
  is_focused_ = false;
  TriggerObserver();
}

void PageObserver::OnWebContentsFocused() {
  is_focused_ = true;
  TriggerObserver();
}

void PageObserver::WebContentsDestroyed() {
  delete this;
}

void PageObserver::TriggerObserver() {
  bool result;
  if (is_fullscreen_ && is_focused_) {
    result = observer_->Activate();
  } else {
    result = observer_->Deactivate();
  }
  if (!result) {
    delete this;
  }
}

}  // namespace keyboard_lock
}  // namespace content
