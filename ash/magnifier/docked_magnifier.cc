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
#include "ui/wm/core/coordinate_conversion.h"

#define FUNC_MARKER_IMPL
#include "my_out.h"

namespace ash {

namespace {

constexpr char kDockedMagnifierTargetWindowName[] =
    "DockedMagnifierTargetWindow";

constexpr int kSplitterHeight = 10;

constexpr float kDefaultScale = 2.0f;

constexpr float kMinScale = 1.0f;
constexpr float kMaxScale = 6.0f;

constexpr float kScrollScaleFactor = 0.0125f;

gfx::Rect GetMagnifierTargetWidgetBounds(aura::Window* root) {
  DCHECK(root);
  DCHECK(root->IsRootWindow());

  auto root_bounds = root->GetBoundsInRootWindow();
  root_bounds.set_height(root_bounds.height() / 4);
  return root_bounds;
}

gfx::Rect GetSplitterLayerBounds(aura::Window* root) {
  DCHECK(root);
  DCHECK(root->IsRootWindow());

  auto root_bounds = root->GetBoundsInRootWindow();
  root_bounds.Offset(0, root_bounds.height() / 4);
  root_bounds.set_height(kSplitterHeight);
  return root_bounds;
}

views::Widget* CreateMagnifierWidget(aura::Window* root) {
  DCHECK(root);
  DCHECK(root->IsRootWindow());

  views::Widget::InitParams params(
      views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
  params.activatable = views::Widget::InitParams::ACTIVATABLE_NO;
  params.accept_events = false;

  params.bounds = GetMagnifierTargetWidgetBounds(root);
  params.opacity = views::Widget::InitParams::OPAQUE_WINDOW;
  params.parent = root;

  auto* widget = new views::Widget;
  widget->Init(params);
  widget->Show();

  aura::Window* window = widget->GetNativeView();
  window->SetName(kDockedMagnifierTargetWindowName);
  return widget;
}

void SetMagnifierHeight(aura::Window* root, int height) {
  DCHECK(root);
  DCHECK(root->IsRootWindow());

  ash::Shelf* shelf = ash::Shelf::ForWindow(root);
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

inline gfx::Point GetCursorScreenPoint() {
  return display::Screen::GetScreen()->GetCursorScreenPoint();
}

double GetMagnifierLayerRotationAngle(
    display::Display::Rotation current_display_active_rotation) {
  switch (current_display_active_rotation) {
    case display::Display::ROTATE_0:
      return 0.0;

    case display::Display::ROTATE_90:
      return -90.0;

    case display::Display::ROTATE_180:
      return -180.0;

    case display::Display::ROTATE_270:
      return -270.0;
  }

  NOTREACHED();
  return 0.0;
}

}  // namespace

DockedMagnifier::DockedMagnifier() : scale_(kDefaultScale) {}

DockedMagnifier::~DockedMagnifier() {}

void DockedMagnifier::SetEnabled(bool enabled) {
  if (enabled_ == enabled)
    return;

  enabled_ = enabled;

  Shell::Get()->system_tray_notifier()->NotifyAccessibilityStatusChanged(
      A11Y_NOTIFICATION_NONE);

  // TODO(afakhry): Make the check mark show. Look into GetAccessibilityState();

  auto* shell = Shell::Get();
  if (enabled) {
    shell->PrependPreTargetHandler(this);
    shell->window_tree_host_manager()->AddObserver(this);
    AdjustMagnifierLayerTransform(GetCursorScreenPoint());
  } else {
    shell->window_tree_host_manager()->RemoveObserver(this);
    shell->RemovePreTargetHandler(this);
    SwitchCurrentSourceRootWindowIfNeeded(nullptr);
  }

  Shell::Get()->UpdateCursorCompositingEnabled();
}

void DockedMagnifier::Toggle() {
  SetEnabled(!enabled_);
}

void DockedMagnifier::OnDisplayConfigurationChanged() {
  if (!enabled_)
    return;

  const auto bounds =
      GetMagnifierTargetWidgetBounds(current_source_root_window_);
  SetMagnifierHeight(current_source_root_window_,
                     bounds.height() + kSplitterHeight);
  target_widget_->SetBounds(bounds);
  magnifier_background_layer_->SetBounds(bounds);
  magnifier_layer_->SetBounds(bounds);
  splitter_layer_->SetBounds(
      GetSplitterLayerBounds(current_source_root_window_));

  AdjustMagnifierLayerTransform(GetCursorScreenPoint());
}

void DockedMagnifier::OnMouseEvent(ui::MouseEvent* event) {
  if (!enabled_)
    return;

  AdjustMagnifierLayerTransform(GetCursorScreenPoint());
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
    AdjustMagnifierLayerTransform(GetCursorScreenPoint());
    event->StopPropagation();
  }
}

void DockedMagnifier::OnCaretBoundsChanged(const ui::TextInputClient* client) {
  DCHECK(enabled_);
  DCHECK(current_source_root_window_);

  const gfx::Rect caret_screen_bounds = client->GetCaretBounds();
  if (caret_screen_bounds.IsEmpty() ||
      !current_source_root_window_->GetBoundsInScreen().Contains(
          caret_screen_bounds)) {
    return;
  }

  AdjustMagnifierLayerTransform(caret_screen_bounds.CenterPoint());
}

void DockedMagnifier::SwitchCurrentSourceRootWindowIfNeeded(
    aura::Window* cursor_root_window) {
  if (current_source_root_window_ == cursor_root_window)
    return;

  aura::Window* old_root_window = current_source_root_window_;
  current_source_root_window_ = cursor_root_window;

  if (old_root_window) {
    SetMagnifierHeight(old_root_window, 0);
    ui::InputMethod* old_input_method =
        GetInputMethodForRootWindow(old_root_window);
    if (old_input_method)
      old_input_method->RemoveObserver(this);
  }

  // TODO: Figure out if we need to observe windows.

  if (reflector_) {
    aura::Env::GetInstance()->context_factory_private()->RemoveReflector(
        reflector_.get());
    reflector_.reset();
  }

  if (splitter_layer_) {
    old_root_window->layer()->Remove(splitter_layer_.get());
    splitter_layer_.reset();
  }

  if (target_widget_) {
    target_widget_->Close();
    target_widget_ = nullptr;
  }

  magnifier_background_layer_.reset();
  magnifier_layer_.reset();

  if (!current_source_root_window_) {
    // No need to create a new magnifier viewport.
    return;
  }

  CreateMagnifierWidgets();

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

void DockedMagnifier::AdjustMagnifierLayerTransform(
    const gfx::Point& magnify_around_screen_point) {
  DCHECK(enabled_);
  DCHECK(magnifier_layer_);

  auto* screen = display::Screen::GetScreen();
  auto* window = screen->GetWindowAtScreenPoint(magnify_around_screen_point);
  if (!window)
    return;

  auto* root_window = window->GetRootWindow();
  DCHECK(root_window);
  SwitchCurrentSourceRootWindowIfNeeded(root_window);

  auto* host = root_window->GetHost();
  DCHECK(host);

  // Adjust the point of interest so that we don't end up magnifying the
  // magnifier.
  const gfx::Rect magnifier_bounds_in_screen =
      GetMagnifierTargetWidgetBounds(current_source_root_window_);
  const gfx::PointF magnifier_center_in_screen(
      magnifier_bounds_in_screen.CenterPoint());

  gfx::Vector3dF scaled_magnifier_bottom_in_pixels(
      0.f, magnifier_bounds_in_screen.bottom() + kSplitterHeight, 0.f);
  host->GetRootTransform().TransformVector(&scaled_magnifier_bottom_in_pixels);
  scaled_magnifier_bottom_in_pixels.Scale(scale_);

  gfx::Vector3dF minimum_height_vector(
      0.f,
      magnifier_center_in_screen.y() +
          scaled_magnifier_bottom_in_pixels.Length(),
      0.f);
  minimum_height_vector.Scale(1 / scale_);
  host->GetInverseRootTransform().TransformVector(&minimum_height_vector);
  const float minimum_height = minimum_height_vector.Length();

  gfx::Point point_of_interest(magnify_around_screen_point);
  ::wm::ConvertPointFromScreen(root_window, &point_of_interest);

  if (point_of_interest.y() < minimum_height)
    point_of_interest.set_y(minimum_height);

  // The point of interest in pixels.
  gfx::PointF point_in_pixels(point_of_interest);
  host->GetRootTransform().TransformPoint(&point_in_pixels);
  point_in_pixels.Scale(scale_);

  // Transform steps: (Note that the transform is applied in the opposite order)
  // 1- Scale the layer by |scale_|.
  // 2- Translate the point of interest back to the origin so that we can rotate
  //    around the Z-axis.
  // 3- Rotate around the Z-axis to undo the effect of screen rotation.
  // 4- Translate the point of interest to the center point of the magnifier
  //    widget.
  gfx::Transform transform;

  // 4- Translate to the center of the magnifier widget.
  transform.Translate(magnifier_center_in_screen.x(),
                      magnifier_center_in_screen.y());

  // 3- Rotate around Z-axis. Account for a possibly rotated screen.
  const int64_t display_id =
      screen->GetDisplayNearestPoint(magnify_around_screen_point).id();
  DCHECK_NE(display_id, display::kInvalidDisplayId);
  const auto& display_info =
      Shell::Get()->display_manager()->GetDisplayInfo(display_id);
  transform.RotateAboutZAxis(
      GetMagnifierLayerRotationAngle(display_info.GetActiveRotation()));

  // 2- Translate back to origin.
  transform.Translate(-point_in_pixels.x(), -point_in_pixels.y());

  // 1- Scale.
  transform.Scale(scale_, scale_);

  ui::ScopedLayerAnimationSettings settings(magnifier_layer_->GetAnimator());
  settings.SetTransitionDuration(base::TimeDelta::FromMilliseconds(0));
  settings.SetTweenType(gfx::Tween::ZERO);
  settings.SetPreemptionStrategy(ui::LayerAnimator::IMMEDIATELY_SET_NEW_TARGET);
  magnifier_layer_->SetTransform(transform);
}

void DockedMagnifier::CreateMagnifierWidgets() {
  DCHECK(current_source_root_window_);

  target_widget_ = CreateMagnifierWidget(current_source_root_window_);

  splitter_layer_ = std::make_unique<ui::Layer>(ui::LAYER_SOLID_COLOR);
  splitter_layer_->SetColor(SK_ColorBLACK);
  splitter_layer_->SetBounds(
      GetSplitterLayerBounds(current_source_root_window_));
  current_source_root_window_->layer()->Add(splitter_layer_.get());

  ui::Layer* target_layer = target_widget_->GetNativeView()->layer();

  magnifier_background_layer_ =
      std::make_unique<ui::Layer>(ui::LAYER_SOLID_COLOR);
  magnifier_background_layer_->SetColor(SK_ColorDKGRAY);
  magnifier_background_layer_->SetMasksToBounds(true);
  const auto viewport_bounds =
      GetMagnifierTargetWidgetBounds(current_source_root_window_);
  magnifier_background_layer_->SetBounds(viewport_bounds);
  target_layer->Add(magnifier_background_layer_.get());

  magnifier_layer_ = std::make_unique<ui::Layer>(ui::LAYER_TEXTURED);
  target_layer->Add(magnifier_layer_.get());
  target_layer->SetMasksToBounds(true);

  const int magnifier_height = viewport_bounds.height();
  SetMagnifierHeight(current_source_root_window_,
                     magnifier_height + kSplitterHeight);
}

}  // namespace ash
