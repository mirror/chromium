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

constexpr char kDockedMagnifierSourceWindowName[] =
    "DockedMagnifierSourceWindow";

constexpr char kDockedMagnifierTargetWindowName[] =
    "DockedMagnifierTargetWindow";

gfx::Rect GetPrimaryDisplayBounds() {
  const auto* display_manager = Shell::Get()->display_manager();
  return display_manager->GetPrimaryDisplayCandidate().bounds();
}

gfx::Rect GetMagnifierTargetWidgetBounds() {
  auto primary_bounds = GetPrimaryDisplayBounds();
  primary_bounds.set_height(primary_bounds.height() / 4);
  return primary_bounds;
}

gfx::Rect GetMagnifierSourceWidgetBounds() {
  gfx::Point cursor_location =
      display::Screen::GetScreen()->GetCursorScreenPoint();
  const auto target_widget_size = GetMagnifierTargetWidgetBounds().size();
  cursor_location.Offset(-target_widget_size.width() / 2,
                         -target_widget_size.height() / 2);
  return gfx::Rect(cursor_location, target_widget_size);
}

aura::Window* GetSourceWidgetRootWindow() {
  auto* screen = display::Screen::GetScreen();
  const auto cursor_location = screen->GetCursorScreenPoint();
  return screen->GetWindowAtScreenPoint(cursor_location)->GetRootWindow();
}

views::Widget* CreateMagnifierWidget(bool is_source_widget) {
  views::Widget::InitParams params(
      views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
  params.activatable = views::Widget::InitParams::ACTIVATABLE_NO;
  params.accept_events = false;

  if (is_source_widget) {
    params.bounds = GetMagnifierSourceWidgetBounds();
    params.opacity = views::Widget::InitParams::TRANSLUCENT_WINDOW;
    params.parent = GetSourceWidgetRootWindow();
  } else {
    params.bounds = GetMagnifierTargetWidgetBounds();
    params.opacity = views::Widget::InitParams::OPAQUE_WINDOW;
    params.parent = Shell::GetPrimaryRootWindow();
  }

  auto* widget = new views::Widget;
  widget->Init(params);
  widget->set_focus_on_creation(false);
  widget->Show();

  aura::Window* window = widget->GetNativeView();
  window->SetName(is_source_widget ? kDockedMagnifierSourceWindowName
                                   : kDockedMagnifierTargetWindowName);
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

  if (enabled) {
    const int magnifier_height = GetMagnifierTargetWidgetBounds().height();
    SetMagnifierHeight(magnifier_height);
    CreateMagnifierWidgets();
  } else {
    SetMagnifierHeight(0);
    CloseMagnifierWidgets();
  }
}

void DockedMagnifier::Toggle() {
  SetEnabled(!enabled_);
}

void DockedMagnifier::CreateMagnifierWidgets() {
  source_widget_ = CreateMagnifierWidget(true /* is_source_widget */);
  target_widget_ = CreateMagnifierWidget(false /* is_source_widget */);

  const auto src_bounds = GetMagnifierSourceWidgetBounds();
  gfx::Transform transform;
  transform.Translate(-src_bounds.x(), -src_bounds.y());
  target_widget_->GetNativeView()->layer()->SetTransform(transform);
  target_widget_->GetNativeView()->layer()->SetMasksToBounds(true);

  CHECK(aura::Env::GetInstance()->context_factory_private());
  reflector_ =
      aura::Env::GetInstance()->context_factory_private()->CreateReflector(
          source_widget_->GetNativeView()->layer()->GetCompositor(),
          target_widget_->GetNativeView()->layer());
}

void DockedMagnifier::CloseMagnifierWidgets() {
  if (reflector_) {
    aura::Env::GetInstance()->context_factory_private()->RemoveReflector(
        reflector_.get());
    reflector_.reset();
  }

  if (source_widget_) {
    source_widget_->Close();
    source_widget_ = nullptr;
  }

  if (target_widget_) {
    target_widget_->Close();
    target_widget_ = nullptr;
  }
}

}  // namespace ash
