// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/pip/pip_window_views.h"

// static
std::unique_ptr<PipWindow> PipWindow::Create() {
  return base::WrapUnique(new PipWindowViews());
}

PipWindowViews::PipWindowViews() : widget_(new views::Widget()) {}

PipWindowViews::~PipWindowViews() {}

void PipWindowViews::Init() {
  // TODO(apacible): Finalize the type of widget.
  views::Widget::InitParams params(views::Widget::InitParams::TYPE_WINDOW);

  // TODO(apacible): Update preferred sizing and positioning.
  params.bounds = gfx::Rect(200, 200, 700, 500);
  params.keep_on_top = true;
  params.visible_on_all_workspaces = true;

  widget_->Init(params);
  widget_->Show();

  // TODO(apacible): Set the WidgetDelegate for more control over behavior.
}

ui::Layer* PipWindowViews::GetLayer() {
  return widget_->GetLayer();
}

bool PipWindowViews::IsActive() const {
  return widget_->IsActive();
}

// Window is never maximized.
bool PipWindowViews::IsMaximized() const {
  return false;
}

// Window is never minimized.
bool PipWindowViews::IsMinimized() const {
  return false;
}

// Window is never fullscreened.
bool PipWindowViews::IsFullscreen() const {
  return false;
}

gfx::NativeWindow PipWindowViews::GetNativeWindow() const {
  return widget_->GetNativeWindow();
}

gfx::Rect PipWindowViews::GetRestoredBounds() const {
  return GetBounds();
}

// Returns the restore state for the window (platform dependent).
ui::WindowShowState PipWindowViews::GetRestoredState() const {
  return ui::SHOW_STATE_NORMAL;
}

// Retrieves the window's current bounds, including its window.
// This will only differ from GetRestoredBounds() for maximized
// and minimized windows.
gfx::Rect PipWindowViews::GetBounds() const {
  return widget_->GetRestoredBounds();
}

void PipWindowViews::Show() {
  widget_->Show();
}

void PipWindowViews::Hide() {
  widget_->Hide();
}

void PipWindowViews::ShowInactive() {
  NOTREACHED();
}

void PipWindowViews::Close() {
  widget_->Close();
}

void PipWindowViews::Activate() {
  widget_->Activate();
}

void PipWindowViews::Deactivate() {
  NOTREACHED();
}

void PipWindowViews::Maximize() {
  NOTREACHED();
}

void PipWindowViews::Minimize() {
  NOTREACHED();
}

void PipWindowViews::Restore() {
  NOTREACHED();
}

void PipWindowViews::SetBounds(const gfx::Rect& bounds) {
  NOTREACHED();
}

void PipWindowViews::FlashFrame(bool flash) {
  NOTREACHED();
}

bool PipWindowViews::IsAlwaysOnTop() const {
  return true;
}

void PipWindowViews::SetAlwaysOnTop(bool always_on_top) {
  NOTREACHED();
}
