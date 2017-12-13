// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/overlay/overlay_window_views.h"

#include "chrome/browser/picture_in_picture/picture_in_picture_window_controller.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

// static
std::unique_ptr<OverlayWindow> OverlayWindow::Create(PictureInPictureWindowController* controller) {
  return base::WrapUnique(new OverlayWindowViews(controller));
}

// TODO: maybe make OverlayWindowViews
class OverlayWidget : public views::Widget {
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
  bool CanResize() const override { return false; }
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

OverlayWindowViews::OverlayWindowViews(PictureInPictureWindowController* controller)
  : OverlayWindow(controller) {
}

OverlayWindowViews::~OverlayWindowViews() = default;

void OverlayWindowViews::Init(const gfx::Size& size) {
  // TODO(apacible): Finalize the type of widget. http://crbug/726621
  views::Widget::InitParams params(
      views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;

  // TODO(apacible): Update preferred sizing and positioning.
  // http://crbug/726621
  params.bounds = gfx::Rect(gfx::Point(200, 200), size);
  params.keep_on_top = true;
  params.visible_on_all_workspaces = true;

  // Set WidgetDelegate for more control...
  params.delegate = new OverlayWindowWidgetDelegate(this);
  views::Widget::Init(params);
  Show();
}

void OverlayWindowViews::OnMouseEvent(ui::MouseEvent* event) {
  if (event->IsOnlyLeftMouseButton() && event->type() == ui::ET_MOUSE_RELEASED) {
    controller_->TogglePlayPause();
    event->SetHandled();
  }
 
  if (event->IsOnlyRightMouseButton() && event->type() == ui::ET_MOUSE_RELEASED) {
    controller_->Close();
    event->SetHandled();
  }
}