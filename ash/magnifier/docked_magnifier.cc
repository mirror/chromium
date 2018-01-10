// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/magnifier/docked_magnifier.h"
#include <ash/shelf/shelf.h>
#include <ash/shelf/shelf_layout_manager.h>
#include <ash/system/tray/system_tray_notifier.h>
#include <ui/aura/env.h>
#include <ui/compositor/layer.h>
#include <ui/display/screen.h>
#include <ui/gfx/geometry/rect.h>
#include <ui/views/widget/widget.h>
#include "ash/shell.h"
#include "ui/compositor/reflector.h"
#include "ui/display/manager/display_manager.h"
#include "ui/gfx/geometry/size.h"

namespace ash {

namespace {

constexpr char kDockedMagnifierTargetWindowName[] =
    "DockedMagnifierTargetWindow";

gfx::Rect GetMagnifierTargetWidgetBounds() {
  auto primary_bounds = Shell::GetPrimaryRootWindow()->GetBoundsInScreen();
  primary_bounds.set_height(primary_bounds.height() / 4);
  return primary_bounds;
}

aura::Window* GetSourceRootWindow() {
  auto* screen = display::Screen::GetScreen();
  const auto cursor_location = screen->GetCursorScreenPoint();
  return screen->GetWindowAtScreenPoint(cursor_location)->GetRootWindow();
}

views::Widget* CreateMagnifierWidget() {
  views::Widget::InitParams params(
      views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
  params.activatable = views::Widget::InitParams::ACTIVATABLE_NO;
  params.accept_events = false;

  params.bounds = GetMagnifierTargetWidgetBounds();
  params.opacity = views::Widget::InitParams::OPAQUE_WINDOW;
  params.parent = Shell::GetPrimaryRootWindow();

  auto* widget = new views::Widget;
  widget->Init(params);
  widget->Show();

  aura::Window* window = widget->GetNativeView();
  window->SetName(kDockedMagnifierTargetWindowName);
  return widget;
}

void SetMagnifierHeight(int height) {
  ash::Shelf* shelf = ash::Shelf::ForWindow(Shell::GetPrimaryRootWindow());
  ash::ShelfLayoutManager* shelf_layout_manager =
      shelf ? shelf->shelf_layout_manager() : nullptr;
  if (shelf_layout_manager)
    shelf_layout_manager->SetDockedMagnifierHeight(height);
}

}  // namespace

DockedMagnifier::DockedMagnifier() {}

DockedMagnifier::~DockedMagnifier() {}

void DockedMagnifier::SetEnabled(bool enabled) {
  enabled_ = enabled;

  // TODO(afakhry): Make the check mark show. Look into GetAccessibilityState();

  auto* shell = Shell::Get();
  if (enabled) {
    const int magnifier_height = GetMagnifierTargetWidgetBounds().height();
    SetMagnifierHeight(magnifier_height);
    CreateMagnifierWidgets();
    shell->AddPreTargetHandler(this);
  } else {
    shell->AddPreTargetHandler(this);
    SetMagnifierHeight(0);
    CloseMagnifierWidgets();
  }
}

void DockedMagnifier::Toggle() {
  SetEnabled(!enabled_);
}

void DockedMagnifier::OnMouseEvent(ui::MouseEvent* event) {
  if (!enabled_)
    return;

  AdjustMagnifierLayerTransform();
}

void DockedMagnifier::CreateMagnifierWidgets() {
  target_widget_ = CreateMagnifierWidget();

  ui::Layer* target_layer = target_widget_->GetNativeView()->layer();

  solid_color_layer_ = std::make_unique<ui::Layer>(ui::LAYER_SOLID_COLOR);
  solid_color_layer_->SetColor(SK_ColorBLACK);
  solid_color_layer_->SetMasksToBounds(true);
  solid_color_layer_->SetBounds(GetMagnifierTargetWidgetBounds());
  target_layer->Add(solid_color_layer_.get());

  magnifier_layer_ = std::make_unique<ui::Layer>(ui::LAYER_TEXTURED);

  AdjustMagnifierLayerTransform();

  target_layer->Add(magnifier_layer_.get());
  target_layer->SetMasksToBounds(true);

  CHECK(aura::Env::GetInstance()->context_factory_private());
  reflector_ =
      aura::Env::GetInstance()->context_factory_private()->CreateReflector(
          GetSourceRootWindow()->layer()->GetCompositor(),
          magnifier_layer_.get());
}

void DockedMagnifier::CloseMagnifierWidgets() {
  if (reflector_) {
    aura::Env::GetInstance()->context_factory_private()->RemoveReflector(
        reflector_.get());
    reflector_.reset();
  }

  if (target_widget_) {
    target_widget_->Close();
    target_widget_ = nullptr;
  }

  solid_color_layer_.reset();

  magnifier_layer_.reset();
}

void DockedMagnifier::AdjustMagnifierLayerTransform() {
  DCHECK(magnifier_layer_);

  constexpr float kMagScale = 2.0f;

  auto cursor_point = display::Screen::GetScreen()->GetCursorScreenPoint();
  cursor_point.set_x(kMagScale * cursor_point.x());
  cursor_point.set_y(kMagScale * cursor_point.y());
  const auto target_center = GetMagnifierTargetWidgetBounds().CenterPoint();
  const auto offset = target_center - cursor_point;

  gfx::Transform transform;
  transform.Translate(offset.x(), offset.y());
  transform.Scale(kMagScale, kMagScale);

  magnifier_layer_->SetTransform(transform);
}

}  // namespace ash
