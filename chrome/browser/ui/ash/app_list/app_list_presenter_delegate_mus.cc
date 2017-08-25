// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/app_list/app_list_presenter_delegate_mus.h"

#include "ui/aura/window.h"
#include "ui/app_list/presenter/app_list_presenter_impl.h"
#include "ui/app_list/presenter/app_list_view_delegate_factory.h"
#include "ui/app_list/views/app_list_view.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/views/mus/mus_client.h"
#include "ui/views/mus/pointer_watcher_event_router.h"
#include "ui/wm/core/window_util.h"

#include "base/strings/stringprintf.h"

namespace {

// Gets the point at the center of the display. The calculation should exclude
// the virtual keyboard area. If the height of the display area is less than
// |minimum_height|, its bottom will be extended to that height (so that the
// app list never starts above the top of the screen).
gfx::Point GetCenterOfDisplay(int64_t display_id, int minimum_height) {
  // TODO(mfomitchev): account for virtual keyboard.
  std::vector<display::Display> displays =
      display::Screen::GetScreen()->GetAllDisplays();
  auto it = std::find_if(displays.begin(), displays.end(),
                         [display_id](const display::Display& display) {
                           return display.id() == display_id;
                         });
  DCHECK(it != displays.end());
  gfx::Rect bounds = it->bounds();

  // Apply the |minimum_height|.
  if (bounds.height() < minimum_height)
    bounds.set_height(minimum_height);

  return bounds.CenterPoint();
}

}  // namespace

AppListPresenterDelegateMus::AppListPresenterDelegateMus(
    app_list::AppListPresenterImpl* presenter,
    app_list::AppListViewDelegateFactory* view_delegate_factory)
    : presenter_(presenter), view_delegate_factory_(view_delegate_factory) {}

AppListPresenterDelegateMus::~AppListPresenterDelegateMus() {
  // Presenter is supposed to be dismissed when the delegate is destroyed. This
  // means we don't have to worry about unregistering the PointerWatcher here.
  DCHECK(!presenter_->GetTargetVisibility());
}

app_list::AppListViewDelegate* AppListPresenterDelegateMus::GetViewDelegate() {
  return view_delegate_factory_->GetDelegate();
}

void AppListPresenterDelegateMus::Init(app_list::AppListView* view,
                                       int64_t display_id,
                                       int current_apps_page) {
  view_ = view;
  // TODO(newcomer): replace Initialize parameters with proper mus
  // implementation as per crbug.com/726838
  // Note: This would place the app list into the USER_WINDOWS container, unlike
  // in classic ash, where it has it's own container.
  // TODO(mfomitchev): We are currently passing NULL for |parent|. It seems like
  // the only thing this is used for is choosing the right scale factor in
  // AppListMainView::PreloadIcons(), so we take care of that - perhaps by
  // passing the display_id or the scale factor directly
  view->Initialize(nullptr /* parent */, current_apps_page, false, false);

  view->MaybeSetAnchorPoint(
      GetCenterOfDisplay(display_id, GetMinimumBoundsHeightForAppList(view)));

  // TODO(mfomitchev): Setup updating bounds on keyboard bounds change.
  // TODO(mfomitchev): Setup dismissing on maximize (touch-view) mode start/end.
  // TODO(mfomitchev): Setup DnD.
  // TODO(mfomitchev): UpdateAutoHideState for shelf
}

void AppListPresenterDelegateMus::OnShown(int64_t display_id) {
  views::MusClient::Get()->pointer_watcher_event_router()->AddPointerWatcher(
      this, false);
  DCHECK(presenter_->GetTargetVisibility());
}

void AppListPresenterDelegateMus::OnDismissed() {
  views::MusClient::Get()->pointer_watcher_event_router()->RemovePointerWatcher(
      this);
  DCHECK(!presenter_->GetTargetVisibility());
}

void AppListPresenterDelegateMus::UpdateBounds() {
  if (!view_ || !presenter_->GetTargetVisibility())
    return;

  view_->UpdateBounds();
}

gfx::Vector2d AppListPresenterDelegateMus::GetVisibilityAnimationOffset(
    aura::Window* root_window) {
  // TODO(mfomitchev): Classic ash does different animation here depending on
  // shelf alignment. We should probably do that too.
  return gfx::Vector2d(0, kAnimationOffset);
}

base::TimeDelta AppListPresenterDelegateMus::GetVisibilityAnimationDuration(
    aura::Window* root_window,
    bool is_visible) {
  // TODO(crbug.com/752695): Classic ash does a different animation here
  // depending on shelf alignment. We should probably do that too.
  return is_visible ? base::TimeDelta::FromMilliseconds(0)
                    : animation_duration();
}

void PrintWindowHierarchy(const aura::Window* active_window,
                          aura::Window* window,
                          int indent,
                          std::ostringstream* out) {
  std::string indent_str(indent, ' ');
  std::string name(window->GetName());
  if (name.empty())
    name = "\"\"";
  *out << indent_str << name << " (" << window << ")"
       << " type=" << window->type()
       << ((window == active_window) ? " [active] " : " ")
       << (window->IsVisible() ? " visible " : " ")
       << window->bounds().ToString()
      //  << (window->GetProperty(kSnapChildrenToPixelBoundary) ? " [snapped] "
      //                                                        : "")
      //  << ", subpixel offset="
      //  << window->layer()->subpixel_position_offset().ToString()
       << '\n';

  for (aura::Window* child : window->children())
    PrintWindowHierarchy(active_window, child, indent + 3, out);
}

void AppListPresenterDelegateMus::OnPointerEventObserved(
    const ui::PointerEvent& event,
    const gfx::Point& location_in_screen,
    gfx::NativeView target) {
  views::Widget* target_widget =
      views::Widget::GetTopLevelWidgetForNativeView(target);
  views::Widget* this_widget = view_ ? view_->GetWidget() : nullptr;
  aura::Window* this_window = this_widget? this_widget->GetNativeWindow() : nullptr;
  std::string parents;
  for (aura::Window* i = target; i; i = wm::GetTransientParent(i)) parents += base::StringPrintf("%p %s, ", i, i->GetName().c_str());
  LOG(ERROR) << "MSW target: " << parents;
  parents = "";
  for (aura::Window* i = target ? target->parent() : nullptr; i; i = wm::GetTransientParent(i)) parents += base::StringPrintf("%p %s, ", i, i->GetName().c_str());
  LOG(ERROR) << "MSW target->parent(): " << parents;
  parents = "";
  for (aura::Window* i = this_window; i; i = i->parent()) parents += base::StringPrintf("%p %s, ", i, i->GetName().c_str());
  LOG(ERROR) << "MSW this:   " << parents;
  LOG(ERROR) << "MSW HasTransientAncestor: " << (this_window && target && wm::HasTransientAncestor(target, this_window)) << " event_target:" << event.target();
  LOG(ERROR) << "MSW target_widget:" << target_widget << " " << (target_widget ? target_widget->GetName() : "") << " this:" << this_widget << " " << (this_widget ? this_widget->GetName() : "");
  parents = "";
  for (aura::Window* i = target; i; i = i->parent()) parents += base::StringPrintf("%p %s, ", i, i->GetName().c_str());
  LOG(ERROR) << "MSW parents: " << parents;

  // MSW target->parent(): 0x1a562b7c87a0 , 0x1a562c857920 AppListView, 
  // MSW this:   0x1a562b36e320 DesktopNativeWidgetAura - content window, 0x1a562c857920 AppListView, 
  
  LOG(ERROR) << "MSW TRANSIENT: " << (this_window && this_window->parent() && target && target->parent() && wm::HasTransientAncestor(target->parent(), this_window->parent()));
  
  // Dismiss app list on a mouse click or touch outside of the app list window.
  if ((event.type() == ui::ET_TOUCH_PRESSED ||
       event.type() == ui::ET_POINTER_DOWN) &&
      (!target || (view_ && target_widget != view_->GetWidget()))) {
    std::ostringstream out_target, out_app_list;
    PrintWindowHierarchy(nullptr, target->GetRootWindow(), 0, &out_target);
    LOG(ERROR) << "MSW WINDOWs (target)\n" << out_target.str();

    PrintWindowHierarchy(nullptr, this_window->GetRootWindow(), 0, &out_app_list);
    LOG(ERROR) << "MSW WINDOWs (app list)\n" << out_app_list.str();

    presenter_->Dismiss();
  }
}
