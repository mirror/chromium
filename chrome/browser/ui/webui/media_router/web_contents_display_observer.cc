// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/media_router/web_contents_display_observer.h"

#include "content/public/browser/web_contents.h"
#include "ui/views/widget/widget.h"

namespace media_router {

WebContentsDisplayObserver::WebContentsDisplayObserver(
    content::WebContents* web_contents,
    base::RepeatingCallback<void()> callback)
    : web_contents_(web_contents),
      widget_(views::Widget::GetWidgetForNativeWindow(
          web_contents->GetTopLevelNativeWindow())),
      callback_(std::move(callback)) {
  display_ = display::Screen::GetScreen()->GetDisplayNearestWindow(
      widget_->GetNativeWindow());
  widget_->AddObserver(this);
  BrowserList::GetInstance()->AddObserver(this);
}

WebContentsDisplayObserver::~WebContentsDisplayObserver() {
  if (widget_) {
    widget_->RemoveObserver(this);
  }
  BrowserList::GetInstance()->RemoveObserver(this);
}

int64_t WebContentsDisplayObserver::GetDisplayId() const {
  return display_.id();
}

void WebContentsDisplayObserver::OnBrowserSetLastActive(Browser* browser) {
  UpdateWidget();
}

void WebContentsDisplayObserver::OnBrowserAdded(Browser* browser) {
  UpdateWidget();
}

void WebContentsDisplayObserver::UpdateWidget() {
  if (views::Widget::GetWidgetForNativeWindow(
          web_contents_->GetTopLevelNativeWindow()) != widget_) {
    if (widget_) {
      widget_->RemoveObserver(this);
    }
    widget_ = views::Widget::GetWidgetForNativeWindow(
        web_contents_->GetTopLevelNativeWindow());
    widget_->AddObserver(this);
  }
}

void WebContentsDisplayObserver::OnWidgetClosing(views::Widget* widget) {
  widget_ = nullptr;
}

void WebContentsDisplayObserver::OnWidgetBoundsChanged(
    views::Widget* widget,
    const gfx::Rect& new_bounds) {
  display::Display new_display =
      display::Screen::GetScreen()->GetDisplayNearestWindow(
          widget_->GetNativeWindow());
  if (new_display.id() == display_.id())
    return;

  display_ = new_display;
  callback_.Run();
}

}  // namespace media_router
