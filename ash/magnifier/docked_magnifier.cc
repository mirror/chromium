// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/magnifier/docked_magnifier.h"
#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_layout_manager.h"
#include "ash/shell.h"
#include "ash/system/tray/system_tray_notifier.h"
#include "base/numerics/ranges.h"
#include "ui/aura/env.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/ime/input_method.h"
#include "ui/base/ime/text_input_client.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/reflector.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/widget/widget.h"

namespace ash {

namespace {

constexpr char kDockedMagnifierTargetWindowName[] =
    "DockedMagnifierTargetWindow";

constexpr int kSplitterHeight = 10;

constexpr float kDefaultScale = 2.0f;

constexpr float kMinScale = 1.0f;
constexpr float kMaxScale = 4.0f;

constexpr float kScrollScaleFactor = 0.0125f;

gfx::Rect GetMagnifierTargetWidgetBounds() {
  auto primary_bounds = Shell::GetPrimaryRootWindow()->GetBoundsInScreen();
  primary_bounds.set_height(primary_bounds.height() / 4);
  return primary_bounds;
}

gfx::Rect GetSplitterLayerBounds() {
  auto primary_bounds = Shell::GetPrimaryRootWindow()->GetBoundsInScreen();
  primary_bounds.Offset(0, primary_bounds.height() / 4);
  primary_bounds.set_height(kSplitterHeight);
  return primary_bounds;
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

ui::InputMethod* GetInputMethodForRootWindow(aura::Window* root_window) {
  DCHECK(root_window);
  auto* host = root_window->GetHost();
  DCHECK(host);
  return host->GetInputMethod();
}

}  // namespace

DockedMagnifier::DockedMagnifier() : scale_(kDefaultScale) {}

DockedMagnifier::~DockedMagnifier() {}

void DockedMagnifier::SetEnabled(bool enabled) {
  if (enabled_ == enabled)
    return;

  enabled_ = enabled;

  // TODO(afakhry): Make the check mark show. Look into GetAccessibilityState();

  auto* shell = Shell::Get();
  if (enabled) {
    const int magnifier_height = GetMagnifierTargetWidgetBounds().height();
    SetMagnifierHeight(magnifier_height + kSplitterHeight);
    CreateMagnifierWidgets();
    shell->AddPreTargetHandler(this);
    shell->window_tree_host_manager()->AddObserver(this);
  } else {
    shell->window_tree_host_manager()->RemoveObserver(this);
    shell->RemovePreTargetHandler(this);
    SetMagnifierHeight(0);
    CloseMagnifierWidgets();
  }

  Shell::Get()->UpdateCursorCompositingEnabled();
}

void DockedMagnifier::Toggle() {
  SetEnabled(!enabled_);
}

void DockedMagnifier::OnDisplayConfigurationChanged() {
  if (!enabled_)
    return;

  const auto bounds = GetMagnifierTargetWidgetBounds();
  SetMagnifierHeight(bounds.height() + kSplitterHeight);
  target_widget_->SetBounds(bounds);
  magnifier_background_layer_->SetBounds(bounds);
  magnifier_layer_->SetBounds(bounds);
  splitter_layer_->SetBounds(GetSplitterLayerBounds());

  AdjustMagnifierLayerTransform();
}

void DockedMagnifier::OnMouseEvent(ui::MouseEvent* event) {
  if (!enabled_)
    return;

  AdjustMagnifierLayerTransform();
}

void DockedMagnifier::OnScrollEvent(ui::ScrollEvent* event) {
  if (!enabled_)
    return;

  if (!event->IsAltDown() || !event->IsControlDown())
    return;

  if (event->type() == ui::ET_SCROLL_FLING_START ||
      event->type() == ui::ET_SCROLL_FLING_CANCEL) {
    event->StopPropagation();
    return;
  }

  if (event->type() == ui::ET_SCROLL) {
    const float y_offset = event->y_offset() * kScrollScaleFactor;
    scale_ = base::ClampToRange(scale_ + y_offset, kMinScale, kMaxScale);
    AdjustMagnifierLayerTransform();
    event->StopPropagation();
  }
}

void DockedMagnifier::OnCaretBoundsChanged(const ui::TextInputClient* client) {
  DCHECK(enabled_);
  DCHECK(current_source_root_window_);

  const gfx::Rect caret_screen_bounds = client->GetCaretBounds();
  if (caret_screen_bounds.IsEmpty())
    return;

  auto* host = current_source_root_window_->GetHost();
  DCHECK(host);

  host->MoveCursorToLocationInDIP(caret_screen_bounds.CenterPoint());
}

void DockedMagnifier::SwitchCurrentSourceRootWindowIfNeeded(
    aura::Window* cursor_root_window) {
  DCHECK(enabled_);

  if (current_source_root_window_ == cursor_root_window)
    return;

  if (current_source_root_window_) {
    ui::InputMethod* old_input_method =
        GetInputMethodForRootWindow(current_source_root_window_);
    if (old_input_method)
      old_input_method->RemoveObserver(this);
  }

  // TODO: Figure out if we need to observe windows.

  if (reflector_) {
    aura::Env::GetInstance()->context_factory_private()->RemoveReflector(
        reflector_.get());
    reflector_.reset();
  }

  current_source_root_window_ = cursor_root_window;

  if (!current_source_root_window_)
    return;

  ui::InputMethod* new_input_method =
      GetInputMethodForRootWindow(current_source_root_window_);
  if (new_input_method)
    new_input_method->AddObserver(this);

  DCHECK(aura::Env::GetInstance()->context_factory_private());
  DCHECK(magnifier_layer_);
  reflector_ =
      aura::Env::GetInstance()->context_factory_private()->CreateReflector(
          current_source_root_window_->layer()->GetCompositor(),
          magnifier_layer_.get());
}

void DockedMagnifier::AdjustMagnifierLayerTransform() {
  DCHECK(enabled_);
  DCHECK(magnifier_layer_);

  auto* screen = display::Screen::GetScreen();
  auto cursor_point = screen->GetCursorScreenPoint();

  auto* cursor_window = screen->GetWindowAtScreenPoint(cursor_point);
  if (!cursor_window)
    return;

  auto* root_window = cursor_window->GetRootWindow();
  DCHECK(root_window);
  SwitchCurrentSourceRootWindowIfNeeded(root_window);

  auto* host = root_window->GetHost();
  DCHECK(host);

  host->ConvertDIPToPixels(&cursor_point);

  cursor_point.set_x(scale_ * cursor_point.x());
  cursor_point.set_y(scale_ * cursor_point.y());

  const auto target_center = GetMagnifierTargetWidgetBounds().CenterPoint();

  const gfx::Vector2d offset = target_center - cursor_point;

  gfx::Transform transform;
  transform.Translate(offset.x(), offset.y());
  transform.Scale(scale_, scale_);

  magnifier_layer_->SetTransform(transform);
}

void DockedMagnifier::CreateMagnifierWidgets() {
  target_widget_ = CreateMagnifierWidget();

  splitter_layer_ = std::make_unique<ui::Layer>(ui::LAYER_SOLID_COLOR);
  splitter_layer_->SetColor(SK_ColorBLACK);
  splitter_layer_->SetBounds(GetSplitterLayerBounds());
  Shell::GetPrimaryRootWindow()->layer()->Add(splitter_layer_.get());

  ui::Layer* target_layer = target_widget_->GetNativeView()->layer();

  magnifier_background_layer_ =
      std::make_unique<ui::Layer>(ui::LAYER_SOLID_COLOR);
  magnifier_background_layer_->SetColor(SK_ColorDKGRAY);
  magnifier_background_layer_->SetMasksToBounds(true);
  magnifier_background_layer_->SetBounds(GetMagnifierTargetWidgetBounds());
  target_layer->Add(magnifier_background_layer_.get());

  magnifier_layer_ = std::make_unique<ui::Layer>(ui::LAYER_TEXTURED);
  target_layer->Add(magnifier_layer_.get());
  target_layer->SetMasksToBounds(true);

  AdjustMagnifierLayerTransform();
}

void DockedMagnifier::CloseMagnifierWidgets() {
  SwitchCurrentSourceRootWindowIfNeeded(nullptr);

  if (splitter_layer_) {
    Shell::GetPrimaryRootWindow()->layer()->Remove(splitter_layer_.get());
    splitter_layer_.reset();
  }

  if (target_widget_) {
    target_widget_->Close();
    target_widget_ = nullptr;
  }

  magnifier_background_layer_.reset();

  magnifier_layer_.reset();
}

}  // namespace ash
