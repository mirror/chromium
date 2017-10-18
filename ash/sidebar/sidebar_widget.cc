// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/sidebar/sidebar_widget.h"

#include "ash/message_center/message_center_view.h"
#include "ash/root_window_controller.h"
#include "ash/screen_util.h"
#include "ash/session/session_controller.h"
#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_widget.h"
#include "ash/shell.h"
#include "ash/system/status_area_widget.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/system/web_notification/web_notification_tray.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/aura/window.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/message_center_style.h"
#include "ui/message_center/views/constants.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/widget/widget_observer.h"

namespace ash {

namespace {

display::Display GetCurrentDisplay(aura::Window* window) {
  return display::Screen::GetScreen()->GetDisplayNearestWindow(window);
}

const SkColor kLightShadingColor = SkColorSetARGB(200, 0, 0, 0);

// TODO(yoshiki): Remove the margins from the sidebar.
const size_t kWidth = message_center::kNotificationWidth +
                      message_center::kMarginBetweenItems * 2;

gfx::Rect CalculateBounds(const gfx::Rect& display_bounds) {
  return gfx::Rect(display_bounds.x() + display_bounds.width() - kWidth,
                   display_bounds.y(), kWidth, display_bounds.height());
}

}  // anonymous namespace

class SidebarWidget::DelegateView : public views::WidgetDelegateView,
                                    public views::ButtonListener {
 public:
  DelegateView(message_center::MessageCenterTray* tray, SidebarMode mode)
      : opaque_background_(ui::LAYER_SOLID_COLOR) {
    opaque_background_.SetBounds(GetLocalBounds());
    opaque_background_.SetColor(kLightShadingColor);

    SetLayoutManager(new views::FillLayout());
    container_ = new views::View();
    layout_ = new views::BoxLayout(views::BoxLayout::kVertical);
    container_->SetLayoutManager(layout_);

    auto* message_center = message_center::MessageCenter::Get();
    bool initially_message_center_settings_visible =
        (mode == SidebarMode::MESSAGE_CENTER_SETTINGS);
    message_center_view_ = new MessageCenterView(
        message_center, tray, 800, initially_message_center_settings_visible);
    message_center_view_->SetNotifications(
        message_center->GetVisibleNotifications());

    container_->AddChildView(message_center_view_);
    layout_->SetFlexForView(message_center_view_, 1);

    container_->SetPaintToLayer();
    container_->layer()->SetFillsBoundsOpaquely(false);
    AddChildView(container_);
  }

  void OnBoundsChanged(const gfx::Rect& old_bounds) override {
    opaque_background_.SetBounds(GetLocalBounds());
  }

  void SetParentLayer(ui::Layer* layer) {
    DCHECK_EQ(layer, GetWidget()->GetLayer());
    layer->Add(&opaque_background_);
    layer->Add(container_->layer());
    ReorderLayers();
  }

  void Layout() override {
    if (!message_center_view_)
      return;

    opaque_background_.SetBounds(GetLocalBounds());
    container_->SetBoundsRect(gfx::Rect(size()));

    int message_center_height = height();
    message_center_view_->SetBounds(0, 0, width(), message_center_height);
  }

  void ReorderChildLayers(ui::Layer* parent_layer) override {
    // may cause crash on launch the OS.
    if (!parent_layer || !GetWidget())
      return;

    if (parent_layer == GetWidget()->GetLayer()) {
      DCHECK_EQ(parent_layer, GetWidget()->GetLayer());
      views::View::ReorderChildLayers(parent_layer);
      parent_layer->StackAtBottom(&opaque_background_);
    }
  }

  void ButtonPressed(views::Button* sender, const ui::Event& event) override {}

 private:
  views::View* container_;
  views::BoxLayout* layout_;
  ash::MessageCenterView* message_center_view_;
  ui::Layer opaque_background_;

  DISALLOW_COPY_AND_ASSIGN(DelegateView);
};

SidebarWidget::SidebarWidget(aura::Window* sidebar_container,
                             Sidebar* sidebar,
                             Shelf* shelf,
                             SidebarMode mode)
    : sidebar_(sidebar),
      shelf_(shelf),
      delegate_view_(new DelegateView(shelf->shelf_widget()
                                          ->status_area_widget()
                                          ->web_notification_tray()
                                          ->GetMessageCenterTray(),
                                      mode)) {
  DCHECK(sidebar_container);
  DCHECK(sidebar_);
  DCHECK(shelf_);

  display::Screen::GetScreen()->AddObserver(this);

  views::Widget::InitParams params(
      views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
  params.name = "SidebarWidget";
  params.opacity = views::Widget::InitParams::TRANSLUCENT_WINDOW;
  params.shadow_type = views::Widget::InitParams::SHADOW_TYPE_DROP;

  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  params.delegate = delegate_view_;
  params.parent = sidebar_container;
  Init(params);

  SetContentsView(delegate_view_);

  gfx::Rect display_bounds = shelf->GetUserWorkAreaBounds();
  SetBounds(CalculateBounds(display_bounds));
  delegate_view_->SetParentLayer(GetLayer());
}

SidebarWidget::~SidebarWidget() {
  display::Screen::GetScreen()->RemoveObserver(this);
}

void SidebarWidget::OnDisplayAdded(const display::Display& new_display) {}

void SidebarWidget::OnDisplayRemoved(const display::Display& old_display) {}

void SidebarWidget::OnDisplayMetricsChanged(const display::Display& display,
                                            uint32_t metrics) {
  if (GetCurrentDisplay(GetNativeView()).id() == display.id()) {
    gfx::Rect display_bounds = shelf_->GetUserWorkAreaBounds();
    SetBounds(CalculateBounds(display_bounds));
  }
}

}  // namespace ash
