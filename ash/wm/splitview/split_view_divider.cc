// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/splitview/split_view_divider.h"

#include "ash/ash_constants.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/shell.h"
#include "ash/wm/window_util.h"
#include "ui/aura/scoped_window_targeter.h"
#include "ui/aura/window_targeter.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/display/display.h"
#include "ui/gfx/canvas.h"
#include "ui/views/view.h"
#include "ui/views/view_targeter_delegate.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/coordinate_conversion.h"
#include "ui/wm/public/activation_client.h"

namespace ash {

namespace {

// The length of short side of the black bar of the divider in dp.
constexpr int kDividerShortSideLength = 7;
constexpr int kDividerEnlargedShortSideLength = 12;

// The length of the white bar of the divider in dp.
constexpr int kWhiteBarShortSideLength = 2;
constexpr int kWhiteBarLongSideLength = 17;
constexpr int kWhiteBarDiameter = 7;
constexpr int kWhiteBarCornerRadius = 1;

// The distance to the divider edge in which a touch gesture will be considered
// as a valid event on the divider.
constexpr int kDividerEdgeInsetForTouch = 5;

// The window targeter that is installed on the always on top container window
// when the split view mode is active.
class AlwaysOnTopWindowTargeter : public aura::WindowTargeter {
 public:
  explicit AlwaysOnTopWindowTargeter(aura::Window* divider_window)
      : divider_window_(divider_window) {}
  ~AlwaysOnTopWindowTargeter() override {}

 private:
  bool GetHitTestRects(aura::Window* target,
                       gfx::Rect* hit_test_rect_mouse,
                       gfx::Rect* hit_test_rect_touch) const override {
    if (target == divider_window_) {
      *hit_test_rect_mouse = *hit_test_rect_touch = gfx::Rect(target->bounds());
      hit_test_rect_touch->Inset(
          gfx::Insets(-kDividerEdgeInsetForTouch, -kDividerEdgeInsetForTouch));
      return true;
    }
    return aura::WindowTargeter::GetHitTestRects(target, hit_test_rect_mouse,
                                                 hit_test_rect_touch);
  }

  aura::Window* divider_window_;

  DISALLOW_COPY_AND_ASSIGN(AlwaysOnTopWindowTargeter);
};

// The divider view class. Passes the mouse/gesture events to the controller.
class DividerView : public views::View, public views::ViewTargeterDelegate {
 public:
  explicit DividerView(SplitViewController* controller)
      : controller_(controller) {
    SetEventTargeter(
        std::unique_ptr<views::ViewTargeter>(new views::ViewTargeter(this)));
  }
  ~DividerView() override {}

  // views::View:
  void OnPaint(gfx::Canvas* canvas) override {
    // Draw the black divider bar.
    canvas->DrawColor(SK_ColorBLACK);

    cc::PaintFlags flags;
    flags.setColor(SK_ColorWHITE);
    flags.setAntiAlias(true);
    if (controller_->is_resizing()) {
      // Draw the white drag circle.
      canvas->DrawCircle(gfx::RectF(GetLocalBounds()).CenterPoint(),
                         kWhiteBarDiameter / 2.f, flags);
    } else {
      // Draw the white drag bar.
      SplitViewController::SnapPosition default_snap_position =
          controller_->default_snap_position();
      if (default_snap_position == SplitViewController::LEFT ||
          default_snap_position == SplitViewController::RIGHT) {
        const gfx::RectF white_bar_bounds(
            (GetLocalBounds().width() - kWhiteBarShortSideLength) / 2.f,
            (GetLocalBounds().height() - kWhiteBarLongSideLength) / 2.f,
            kWhiteBarShortSideLength, kWhiteBarLongSideLength);
        canvas->DrawRoundRect(white_bar_bounds, kWhiteBarCornerRadius, flags);
      } else {
        DCHECK(default_snap_position == SplitViewController::TOP ||
               default_snap_position == SplitViewController::BOTTOM);
        const gfx::RectF white_bar_bounds(
            (GetLocalBounds().width() - kWhiteBarLongSideLength) / 2.f,
            (GetLocalBounds().height() - kWhiteBarShortSideLength) / 2.f,
            kWhiteBarLongSideLength, kWhiteBarShortSideLength);
        canvas->DrawRoundRect(white_bar_bounds, kWhiteBarCornerRadius, flags);
      }
    }
  }

  bool OnMousePressed(const ui::MouseEvent& event) override {
    gfx::Point location(event.location());
    views::View::ConvertPointToScreen(this, &location);
    controller_->StartResize(location);
    return true;
  }

  bool OnMouseDragged(const ui::MouseEvent& event) override {
    gfx::Point location(event.location());
    views::View::ConvertPointToScreen(this, &location);
    controller_->Resize(location);
    return true;
  }

  void OnMouseReleased(const ui::MouseEvent& event) override {
    gfx::Point location(event.location());
    views::View::ConvertPointToScreen(this, &location);
    controller_->EndResize(location);
  }

  void OnGestureEvent(ui::GestureEvent* event) override {
    gfx::Point location(event->location());
    views::View::ConvertPointToScreen(this, &location);
    switch (event->type()) {
      case ui::ET_GESTURE_TAP_DOWN:
      case ui::ET_GESTURE_SCROLL_BEGIN:
        controller_->StartResize(location);
        break;
      case ui::ET_GESTURE_SCROLL_UPDATE:
        controller_->Resize(location);
        break;
      case ui::ET_GESTURE_END:
        controller_->EndResize(location);
        break;
      default:
        break;
    }
    event->SetHandled();
  }

  // views::ViewTargeterDelegate:
  bool DoesIntersectRect(const views::View* target,
                         const gfx::Rect& rect) const override {
    DCHECK_EQ(target, this);
    return true;
  }

 private:
  SplitViewController* controller_;

  DISALLOW_COPY_AND_ASSIGN(DividerView);
};

}  // namespace

SplitViewDivider::SplitViewDivider(SplitViewController* controller,
                                   aura::Window* root_window)
    : controller_(controller) {
  DCHECK(root_window);
  Shell::Get()->activation_client()->AddObserver(this);
  CreateDividerWidget(root_window);

  aura::Window* always_on_top_container =
      Shell::GetContainer(root_window, kShellWindowId_AlwaysOnTopContainer);
  split_view_window_targeter_ = base::MakeUnique<aura::ScopedWindowTargeter>(
      always_on_top_container, base::MakeUnique<AlwaysOnTopWindowTargeter>(
                                   divider_widget_->GetNativeWindow()));
}

SplitViewDivider::~SplitViewDivider() {
  Shell::Get()->activation_client()->RemoveObserver(this);
  divider_widget_.reset();
  split_view_window_targeter_.reset();
  for (auto* iter : observed_windows_)
    iter->RemoveObserver(this);
}

// static
gfx::Size SplitViewDivider::GetDividerSize(
    const gfx::Rect& work_area_bounds,
    SplitViewController::SnapPosition snap_position,
    bool is_dragging) {
  if (snap_position == SplitViewController::LEFT ||
      snap_position == SplitViewController::RIGHT) {
    return is_dragging
               ? gfx::Size(kDividerEnlargedShortSideLength,
                           work_area_bounds.height())
               : gfx::Size(kDividerShortSideLength, work_area_bounds.height());
  } else if (snap_position == SplitViewController::TOP ||
             snap_position == SplitViewController::BOTTOM) {
    return is_dragging
               ? gfx::Size(work_area_bounds.width(),
                           kDividerEnlargedShortSideLength)
               : gfx::Size(work_area_bounds.width(), kDividerShortSideLength);
  }
  return gfx::Size();
}

void SplitViewDivider::UpdateDividerBounds(bool is_dragging) {
  divider_widget_->SetBounds(GetDividerBoundsInScreen(is_dragging));
}

gfx::Rect SplitViewDivider::GetDividerBoundsInScreen(bool is_dragging) {
  SplitViewController::SnapPosition default_snap_position =
      controller_->default_snap_position();
  if (default_snap_position == SplitViewController::NONE)
    return gfx::Rect();

  aura::Window* root_window =
      divider_widget_->GetNativeWindow()->GetRootWindow();
  const gfx::Rect work_area_bounds_in_screen =
      controller_->GetDisplayWorkAreaBoundsInScreen(root_window);
  const int divider_position = controller_->divider_position();
  const gfx::Size divider_size =
      GetDividerSize(controller_->GetDisplayWorkAreaBoundsInScreen(root_window),
                     default_snap_position, is_dragging);

  if (default_snap_position == SplitViewController::LEFT ||
      default_snap_position == SplitViewController::RIGHT) {
    return is_dragging
               ? gfx::Rect(divider_position - (kDividerEnlargedShortSideLength -
                                               kDividerShortSideLength) /
                                                  2,
                           work_area_bounds_in_screen.y(), divider_size.width(),
                           divider_size.height())
               : gfx::Rect(divider_position, work_area_bounds_in_screen.y(),
                           divider_size.width(), divider_size.height());
  } else {
    DCHECK(default_snap_position == SplitViewController::TOP ||
           default_snap_position == SplitViewController::BOTTOM);
    return is_dragging
               ? gfx::Rect(work_area_bounds_in_screen.x(),
                           divider_position - (kDividerEnlargedShortSideLength -
                                               kDividerShortSideLength) /
                                                  2,
                           divider_size.width(), divider_size.height())
               : gfx::Rect(work_area_bounds_in_screen.x(), divider_position,
                           divider_size.width(), divider_size.height());
  }
}

void SplitViewDivider::AddObservedWindow(aura::Window* window) {
  auto iter =
      std::find(observed_windows_.begin(), observed_windows_.end(), window);
  if (iter == observed_windows_.end()) {
    window->AddObserver(this);
    observed_windows_.push_back(window);
  }
}

void SplitViewDivider::RemoveObservedWindow(aura::Window* window) {
  auto iter =
      std::find(observed_windows_.begin(), observed_windows_.end(), window);
  if (iter != observed_windows_.end()) {
    window->RemoveObserver(this);
    observed_windows_.erase(iter);
  }
}

void SplitViewDivider::OnWindowDestroying(aura::Window* window) {
  RemoveObservedWindow(window);
}

void SplitViewDivider::OnWindowActivated(ActivationReason reason,
                                         aura::Window* gained_active,
                                         aura::Window* lost_active) {
  auto iter = std::find(observed_windows_.begin(), observed_windows_.end(),
                        gained_active);
  if (iter != observed_windows_.end()) {
    divider_widget_->SetAlwaysOnTop(true);
  } else {
    divider_widget_->SetAlwaysOnTop(false);
    divider_widget_->Deactivate();
  }
}

void SplitViewDivider::CreateDividerWidget(aura::Window* root_window) {
  DCHECK(!divider_widget_.get());
  divider_widget_ = base::MakeUnique<views::Widget>();
  views::Widget::InitParams params(views::Widget::InitParams::TYPE_POPUP);
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  params.opacity = views::Widget::InitParams::OPAQUE_WINDOW;
  params.parent =
      Shell::GetContainer(root_window, kShellWindowId_AlwaysOnTopContainer);
  DividerView* divider_view = new DividerView(controller_);
  divider_widget_->set_focus_on_creation(false);
  divider_widget_->Init(params);
  divider_widget_->SetContentsView(divider_view);
  divider_widget_->SetBounds(GetDividerBoundsInScreen(false /* is_dragging */));
  divider_widget_->Show();
}

}  // namespace ash
