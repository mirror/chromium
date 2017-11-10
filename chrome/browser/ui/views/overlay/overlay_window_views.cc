// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/overlay/overlay_window_views.h"

#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/screen.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

// static
std::unique_ptr<OverlayWindow> OverlayWindow::Create() {
  return base::WrapUnique(new OverlayWindowViews());
}

namespace {

// OverlayWindow implementation of Widget.
class OverlayWindowWidget : public views::Widget {
 public:
  OverlayWindowWidget() {}

  // views::Widget:
  gfx::Size GetMinimumSize() const override {
    // TODO(apacible): Update minimum size after more feedback.
    // Initial value 140 taken from PRD.
    return gfx::Size(140, 140);
  }
  gfx::Size GetMaximumSize() const override {
    // Currently maximizes to be a little smaller than one quadrant of the
    // primary display work area.
    gfx::Rect work_area =
        display::Screen::GetScreen()->GetPrimaryDisplay().work_area();
    return gfx::Size(work_area.width() / 3, work_area.height() / 3);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(OverlayWindowWidget);
};

// OverlayWindow implementation of WidgetDelegate.
class OverlayWindowWidgetDelegate : public views::WidgetDelegate {
 public:
  explicit OverlayWindowWidgetDelegate(views::Widget* widget)
      : widget_(widget) {
    DCHECK(widget_);
  }
  ~OverlayWindowWidgetDelegate() override = default;

  // WidgetDelegate:
  bool CanResize() const override { return true; }
  ui::ModalType GetModalType() const override { return ui::MODAL_TYPE_SYSTEM; }
  base::string16 GetWindowTitle() const override {
    return l10n_util::GetStringUTF16(IDS_PICTURE_IN_PICTURE_TITLE_TEXT);
  }
  views::Widget* GetWidget() override { return widget_; }
  const views::Widget* GetWidget() const override { return widget_; }

 private:
  // Owns OverlayWindowWidgetDelegate.
  views::Widget* widget_;

  DISALLOW_COPY_AND_ASSIGN(OverlayWindowWidgetDelegate);
};

}  // namespace

OverlayWindowViews::OverlayWindowViews() {
  widget_.reset(new OverlayWindowWidget());
}

OverlayWindowViews::~OverlayWindowViews() = default;

void OverlayWindowViews::Init() {
  // TODO(apacible): Finalize the type of widget. http://crbug/726621
  views::Widget::InitParams params(views::Widget::InitParams::TYPE_WINDOW);
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;

  // TODO(apacible): Update preferred sizing and positioning.
  // http://crbug/726621
  // These bounds are arbitrary. See OverlayWindowWidget for specified
  // constraints. The initial positioning is on the bottom right quadrant
  // of the primary display work area.
  gfx::Rect work_area =
      display::Screen::GetScreen()->GetPrimaryDisplay().work_area();
  params.bounds =
      gfx::Rect(work_area.width() / 2, work_area.height() / 2, 500, 300);
  params.keep_on_top = true;
  params.visible_on_all_workspaces = true;

  // Set WidgetDelegate for more control over |widget_|.
  params.delegate = new OverlayWindowWidgetDelegate(widget_.get());

  widget_->Init(params);
  widget_->Show();
}

bool OverlayWindowViews::IsActive() const {
  return widget_->IsActive();
}

void OverlayWindowViews::Show() {
  widget_->Show();
}

void OverlayWindowViews::Hide() {
  widget_->Hide();
}

void OverlayWindowViews::Close() {
  widget_->Close();
}

void OverlayWindowViews::Activate() {
  widget_->Activate();
}

bool OverlayWindowViews::IsAlwaysOnTop() const {
  return true;
}

ui::Layer* OverlayWindowViews::GetLayer() {
  return widget_->GetLayer();
}

gfx::NativeWindow OverlayWindowViews::GetNativeWindow() const {
  return widget_->GetNativeWindow();
}

gfx::Rect OverlayWindowViews::GetBounds() const {
  return widget_->GetRestoredBounds();
}
