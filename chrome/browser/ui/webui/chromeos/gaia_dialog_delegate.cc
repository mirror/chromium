// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/gaia_dialog_delegate.h"

#include "chrome/browser/chromeos/login/ui/login_display_host_views.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"

namespace chromeos {

namespace {

const char kGaiaURL[] = "chrome://oobe/gaia-signin";
const int kGaiaDialogHeight = 640;
const int kGaiaDialogWidth = 768;

}  // namespace

GaiaDialogDelegate::GaiaDialogDelegate(
    base::WeakPtr<LoginDisplayHostViews> controller)
    : controller_(controller) {
  size_.SetSize(kGaiaDialogWidth, kGaiaDialogHeight);
}

GaiaDialogDelegate::~GaiaDialogDelegate() {
  if (controller_)
    controller_->OnDialogDestroyed(this);
}

void GaiaDialogDelegate::Close() {
  if (widget_)
    widget_->Close();
}

void GaiaDialogDelegate::SetWidget(views::Widget* widget) {
  widget_ = widget;
}

void GaiaDialogDelegate::SetSize(int width, int height) {
  if (size_.width() == width && size_.height() == height)
    return;

  size_.SetSize(width, height);
  if (widget_) {
    const gfx::Rect rect =
        display::Screen::GetScreen()
            ->GetDisplayNearestWindow(widget_->GetNativeWindow())
            .work_area();
    gfx::Rect bounds(rect.x() + (rect.width() - size_.width()) / 2,
                     rect.y() + (rect.height() - size_.height()) / 2,
                     size_.width(), size_.height());
    widget_->SetBounds(bounds);
  }
}

ui::ModalType GaiaDialogDelegate::GetDialogModalType() const {
  return ui::MODAL_TYPE_SYSTEM;
}

base::string16 GaiaDialogDelegate::GetDialogTitle() const {
  return base::string16();
}

GURL GaiaDialogDelegate::GetDialogContentURL() const {
  return GURL(kGaiaURL);
}

void GaiaDialogDelegate::GetWebUIMessageHandlers(
    std::vector<content::WebUIMessageHandler*>* handlers) const {}

void GaiaDialogDelegate::GetDialogSize(gfx::Size* size) const {
  size->SetSize(size_.width(), size_.height());
}

bool GaiaDialogDelegate::CanResizeDialog() const {
  return false;
}

std::string GaiaDialogDelegate::GetDialogArgs() const {
  return std::string();
}

void GaiaDialogDelegate::OnDialogClosed(const std::string& json_retval) {
  delete this;
}

void GaiaDialogDelegate::OnCloseContents(content::WebContents* source,
                                         bool* out_close_dialog) {
  *out_close_dialog = true;
}

bool GaiaDialogDelegate::ShouldShowDialogTitle() const {
  return false;
}

bool GaiaDialogDelegate::HandleContextMenu(
    const content::ContextMenuParams& params) {
  return true;
}

std::vector<ui::Accelerator> GaiaDialogDelegate::GetAccelerators() {
  // TODO: Adding necessory accelerators.
  return std::vector<ui::Accelerator>();
}

}  // namespace chromeos
