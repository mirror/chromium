// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/desktop_background/desktop_background_view.h"

#include <limits>

#include "ash/ash_export.h"
#include "ash/desktop_background/desktop_background_controller.h"
#include "ash/shell.h"
#include "ash/shell_window_ids.h"
#include "ash/wm/root_window_layout_manager.h"
#include "ash/wm/window_animations.h"
#include "base/message_loop.h"
#include "base/utf_string_conversions.h"
#include "grit/ui_resources.h"
#include "ui/aura/root_window.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/layer_animation_observer.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/image.h"
#include "ui/views/widget/widget.h"

namespace ash {
namespace internal {
namespace {

class ShowWallpaperAnimationObserver : public ui::ImplicitAnimationObserver {
 public:
  ShowWallpaperAnimationObserver(aura::RootWindow* root_window,
                                 views::Widget* desktop_widget)
      : root_window_(root_window),
        desktop_widget_(desktop_widget) {
  }

  virtual ~ShowWallpaperAnimationObserver() {
  }

 private:
  // Overridden from ui::ImplicitAnimationObserver:
  virtual void OnImplicitAnimationsCompleted() OVERRIDE {
    internal::RootWindowLayoutManager* root_window_layout =
        static_cast<internal::RootWindowLayoutManager*>(
            root_window_->layout_manager());
    root_window_layout->SetBackgroundWidget(desktop_widget_);

    ash::Shell::GetInstance()->
        user_wallpaper_delegate()->OnWallpaperAnimationFinished();

    MessageLoop::current()->DeleteSoon(FROM_HERE, this);
  }

  aura::RootWindow* root_window_;
  views::Widget* desktop_widget_;

  DISALLOW_COPY_AND_ASSIGN(ShowWallpaperAnimationObserver);
};

}  // namespace

// For our scaling ratios we need to round positive numbers.
static int RoundPositive(double x) {
  return static_cast<int>(floor(x + 0.5));
}

////////////////////////////////////////////////////////////////////////////////
// DesktopBackgroundView, public:

DesktopBackgroundView::DesktopBackgroundView() {
}

DesktopBackgroundView::~DesktopBackgroundView() {
}

////////////////////////////////////////////////////////////////////////////////
// DesktopBackgroundView, views::View overrides:

void DesktopBackgroundView::OnPaint(gfx::Canvas* canvas) {
  // Scale the image while maintaining the aspect ratio, cropping as
  // necessary to fill the background. Ideally the image should be larger
  // than the largest display supported, if not we will center it rather than
  // streching to avoid upsampling artifacts (Note that we could tile too, but
  // decided not to do this at the moment).
  DesktopBackgroundController* controller = ash::Shell::GetInstance()->
      desktop_background_controller();
  gfx::ImageSkia wallpaper = controller->GetWallpaper();
  WallpaperLayout wallpaper_layout = controller->GetWallpaperLayout();

  gfx::Rect wallpaper_rect(0, 0, wallpaper.width(), wallpaper.height());
  if (wallpaper_layout == ash::CENTER_CROPPED && wallpaper.width() > width()
      && wallpaper.height() > height()) {
    // The dimension with the smallest ratio must be cropped, the other one
    // is preserved. Both are set in gfx::Size cropped_size.
    double horizontal_ratio = static_cast<double>(width()) /
        static_cast<double>(wallpaper.width());
    double vertical_ratio = static_cast<double>(height()) /
        static_cast<double>(wallpaper.height());

    gfx::Size cropped_size;
    if (vertical_ratio > horizontal_ratio) {
      cropped_size = gfx::Size(
          RoundPositive(static_cast<double>(width()) / vertical_ratio),
          wallpaper.height());
    } else {
      cropped_size = gfx::Size(wallpaper.width(),
          RoundPositive(static_cast<double>(height()) / horizontal_ratio));
    }

    gfx::Rect wallpaper_cropped_rect = wallpaper_rect.Center(cropped_size);
    canvas->DrawImageInt(wallpaper,
        wallpaper_cropped_rect.x(), wallpaper_cropped_rect.y(),
        wallpaper_cropped_rect.width(), wallpaper_cropped_rect.height(),
        0, 0, width(), height(),
        true);
  } else if (wallpaper_layout == ash::TILE) {
    canvas->TileImageInt(wallpaper, 0, 0, width(), height());
  } else if (wallpaper_layout == ash::STRETCH) {
    // This is generally not recommended as it may show artifacts.
    canvas->DrawImageInt(wallpaper, 0, 0, wallpaper.width(),
        wallpaper.height(), 0, 0, width(), height(), true);
  } else {
    // All other are simply centered, and not scaled (but may be clipped).
     canvas->DrawImageInt(wallpaper, (width() - wallpaper.width()) / 2,
         (height() - wallpaper.height()) / 2);
  }
}

bool DesktopBackgroundView::OnMousePressed(const views::MouseEvent& event) {
  return true;
}

void DesktopBackgroundView::OnMouseReleased(const views::MouseEvent& event) {
  if (event.IsRightMouseButton())
    Shell::GetInstance()->ShowBackgroundMenu(GetWidget(), event.location());
}

void CreateDesktopBackground(aura::RootWindow* root_window) {
  DesktopBackgroundController* controller = ash::Shell::GetInstance()->
      desktop_background_controller();
  views::Widget* desktop_widget = new views::Widget;
  views::Widget::InitParams params(
      views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
  DesktopBackgroundView* view = new DesktopBackgroundView();
  params.delegate = view;
  if (controller->GetWallpaper().empty())
    params.transparent = true;
  params.parent = root_window->GetChildById(
      ash::internal::kShellWindowId_DesktopBackgroundContainer);
  desktop_widget->Init(params);
  desktop_widget->SetContentsView(view);
  ash::SetWindowVisibilityAnimationType(
      desktop_widget->GetNativeView(),
      ash::WINDOW_VISIBILITY_ANIMATION_TYPE_FADE);
  ash::SetWindowVisibilityAnimationTransition(
      desktop_widget->GetNativeView(),
      ash::ANIMATE_SHOW);
  desktop_widget->SetBounds(params.parent->bounds());
  ui::ScopedLayerAnimationSettings settings(desktop_widget->GetNativeView()->
                                            layer()->GetAnimator());
  settings.SetPreemptionStrategy(ui::LayerAnimator::ENQUEUE_NEW_ANIMATION);
  settings.AddObserver(new ShowWallpaperAnimationObserver(root_window,
                                                          desktop_widget));
  desktop_widget->Show();
  desktop_widget->GetNativeView()->SetName("DesktopBackgroundView");
}

}  // namespace internal
}  // namespace ash
