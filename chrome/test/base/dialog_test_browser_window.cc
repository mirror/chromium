// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/dialog_test_browser_window.h"

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/web_contents.h"
#include "ui/views/widget/widget.h"

using web_modal::WebContentsModalDialogHost;
using web_modal::ModalDialogHostObserver;

DialogTestBrowserWindow::DialogTestBrowserWindow(gfx::NativeWindow context) {
  host_window_.reset(new views::Widget);
  views::Widget::InitParams params(views::Widget::InitParams::TYPE_WINDOW);
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  params.context = context;
  host_window_->Init(params);
  // Leave the window hidden: unit tests shouldn't need it to be visible.
}

DialogTestBrowserWindow::~DialogTestBrowserWindow() {
}


WebContentsModalDialogHost*
DialogTestBrowserWindow::GetWebContentsModalDialogHost() {
  return this;
}

gfx::NativeView DialogTestBrowserWindow::GetHostView() const {
  return host_window_->GetNativeView();
}

gfx::Point DialogTestBrowserWindow::GetDialogPosition(const gfx::Size& size) {
  return gfx::Point();
}

gfx::Size DialogTestBrowserWindow::GetMaximumDialogSize() {
  return gfx::Size();
}

void DialogTestBrowserWindow::AddObserver(ModalDialogHostObserver* observer) {
}

void DialogTestBrowserWindow::RemoveObserver(
    ModalDialogHostObserver* observer) {
}
