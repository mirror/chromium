// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/media_router/web_contents_display_observer.h"

#include "content/public/browser/web_contents.h"

namespace media_router {

WebContentsDisplayObserver::WebContentsDisplayObserver(
    content::WebContents* web_contents,
    base::RepeatingCallback<void()> callback)
    : web_contents_(web_contents),
      window_(web_contents->GetTopLevelNativeWindow()),
      callback_(std::move(callback)) {
  display_ = display::Screen::GetScreen()->GetDisplayNearestWindow(window_);
  window_->GetHost()->AddObserver(this);
  window_->AddObserver(this);
  BrowserList::GetInstance()->AddObserver(this);
}

WebContentsDisplayObserver::~WebContentsDisplayObserver() {
  if (window_) {
    window_->GetHost()->RemoveObserver(this);
    window_->RemoveObserver(this);
  }
  BrowserList::GetInstance()->RemoveObserver(this);
}

void WebContentsDisplayObserver::OnHostMovedInPixels(
    aura::WindowTreeHost* host,
    const gfx::Point& new_origin_in_pixels) {
  display::Display new_display =
      display::Screen::GetScreen()->GetDisplayNearestWindow(window_);
  if (new_display.id() == display_.id())
    return;

  display_ = new_display;
  callback_.Run();
}

int64_t WebContentsDisplayObserver::GetDisplayId() const {
  return display_.id();
}

void WebContentsDisplayObserver::OnBrowserSetLastActive(Browser* browser) {
  UpdateWindow();
}

void WebContentsDisplayObserver::OnBrowserAdded(Browser* browser) {
  UpdateWindow();
}

void WebContentsDisplayObserver::OnWindowDestroying(aura::Window* window) {
  window_ = nullptr;
}

void WebContentsDisplayObserver::UpdateWindow() {
  if (web_contents_->GetTopLevelNativeWindow() != window_) {
    if (window_) {
      window_->GetHost()->RemoveObserver(this);
      window_->RemoveObserver(this);
    }
    window_ = web_contents_->GetTopLevelNativeWindow();
    window_->GetHost()->AddObserver(this);
    window_->AddObserver(this);
  }
}

}  // namespace media_router
