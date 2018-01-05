// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/overlay/overlay_window_views.h"

#include "chrome/browser/picture_in_picture/picture_in_picture_window_controller.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/screen.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

// static
std::unique_ptr<OverlayWindow> OverlayWindow::Create(
    PictureInPictureWindowController* controller) {
  return base::WrapUnique(new OverlayWindowViews(controller));
}

// TODO: maybe make OverlayWindowViews
class OverlayWidget : public views::Widget {};

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

OverlayWindowViews::OverlayWindowViews(
    PictureInPictureWindowController* controller)
    : OverlayWindow(controller) {}

OverlayWindowViews::~OverlayWindowViews() = default;

void OverlayWindowViews::Init(const gfx::Size& size) {
  views::Widget::InitParams params(
      views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;

  // These bounds are arbitrary. See OverlayWindowWidget for specified
  // constraints. The initial positioning is on the bottom right quadrant
  // of the primary display work area.
  gfx::Rect work_area =
      display::Screen::GetScreen()->GetPrimaryDisplay().work_area();
  int window_diff_width = work_area.width() - size.width();
  int window_diff_height = work_area.height() - size.height();
  // 2% of the average of the window diffs.
  int buffer = (window_diff_width + window_diff_height) / 2 * 0.02;
  params.bounds = gfx::Rect(
      gfx::Point(window_diff_width - buffer, window_diff_height - buffer),
      size);

  params.keep_on_top = true;
  params.visible_on_all_workspaces = true;

  // Set WidgetDelegate for more control...
  params.delegate = new OverlayWindowWidgetDelegate(this);
  views::Widget::Init(params);
  Show();
}
