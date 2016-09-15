// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/common/wm_root_window_controller.h"

#include "ash/common/session/session_state_delegate.h"
#include "ash/common/shelf/wm_shelf.h"
#include "ash/common/shell_delegate.h"
#include "ash/common/shell_window_ids.h"
#include "ash/common/wallpaper/wallpaper_delegate.h"
#include "ash/common/wallpaper/wallpaper_widget_controller.h"
#include "ash/common/wm/container_finder.h"
#include "ash/common/wm/root_window_layout_manager.h"
#include "ash/common/wm/system_modal_container_layout_manager.h"
#include "ash/common/wm/window_state.h"
#include "ash/common/wm/workspace/workspace_layout_manager.h"
#include "ash/common/wm/workspace_controller.h"
#include "ash/common/wm_shell.h"
#include "ash/common/wm_window.h"
#include "base/memory/ptr_util.h"
#include "ui/base/models/menu_model.h"
#include "ui/views/controls/menu/menu_model_adapter.h"
#include "ui/views/controls/menu/menu_runner.h"

namespace ash {
namespace {

// Scales |value| that is originally between 0 and |src_max| to be between
// 0 and |dst_max|.
float ToRelativeValue(int value, int src_max, int dst_max) {
  return static_cast<float>(value) / static_cast<float>(src_max) * dst_max;
}

// Uses ToRelativeValue() to scale the origin of |bounds_in_out|. The
// width/height are not changed.
void MoveOriginRelativeToSize(const gfx::Size& src_size,
                              const gfx::Size& dst_size,
                              gfx::Rect* bounds_in_out) {
  gfx::Point origin = bounds_in_out->origin();
  bounds_in_out->set_origin(gfx::Point(
      ToRelativeValue(origin.x(), src_size.width(), dst_size.width()),
      ToRelativeValue(origin.y(), src_size.height(), dst_size.height())));
}

// Reparents |window| to |new_parent|.
void ReparentWindow(WmWindow* window, WmWindow* new_parent) {
  const gfx::Size src_size = window->GetParent()->GetBounds().size();
  const gfx::Size dst_size = new_parent->GetBounds().size();
  // Update the restore bounds to make it relative to the display.
  wm::WindowState* state = window->GetWindowState();
  gfx::Rect restore_bounds;
  bool has_restore_bounds = state->HasRestoreBounds();

  bool update_bounds =
      (state->IsNormalOrSnapped() || state->IsMinimized()) &&
      new_parent->GetShellWindowId() != kShellWindowId_DockedContainer;
  gfx::Rect local_bounds;
  if (update_bounds) {
    local_bounds = state->window()->GetBounds();
    MoveOriginRelativeToSize(src_size, dst_size, &local_bounds);
  }

  if (has_restore_bounds) {
    restore_bounds = state->GetRestoreBoundsInParent();
    MoveOriginRelativeToSize(src_size, dst_size, &restore_bounds);
  }

  new_parent->AddChild(window);

  // Docked windows have bounds handled by the layout manager in AddChild().
  if (update_bounds)
    window->SetBounds(local_bounds);

  if (has_restore_bounds)
    state->SetRestoreBoundsInParent(restore_bounds);
}

// Reparents the appropriate set of windows from |src| to |dst|.
void ReparentAllWindows(WmWindow* src, WmWindow* dst) {
  // Set of windows to move.
  const int kContainerIdsToMove[] = {
      kShellWindowId_DefaultContainer,
      kShellWindowId_DockedContainer,
      kShellWindowId_PanelContainer,
      kShellWindowId_AlwaysOnTopContainer,
      kShellWindowId_SystemModalContainer,
      kShellWindowId_LockSystemModalContainer,
      kShellWindowId_UnparentedControlContainer,
      kShellWindowId_OverlayContainer,
  };
  const int kExtraContainerIdsToMoveInUnifiedMode[] = {
      kShellWindowId_LockScreenContainer,
      kShellWindowId_LockScreenWallpaperContainer,
  };
  std::vector<int> container_ids(
      kContainerIdsToMove,
      kContainerIdsToMove + arraysize(kContainerIdsToMove));
  // Check the display mode as this is also necessary when trasitioning between
  // mirror and unified mode.
  if (WmShell::Get()->IsInUnifiedModeIgnoreMirroring()) {
    for (int id : kExtraContainerIdsToMoveInUnifiedMode)
      container_ids.push_back(id);
  }

  for (int id : container_ids) {
    WmWindow* src_container = src->GetChildByShellWindowId(id);
    WmWindow* dst_container = dst->GetChildByShellWindowId(id);
    while (!src_container->GetChildren().empty()) {
      // Restart iteration from the source container windows each time as they
      // may change as a result of moving other windows.
      WmWindow::Windows src_container_children = src_container->GetChildren();
      WmWindow::Windows::const_iterator iter = src_container_children.begin();
      while (iter != src_container_children.end() &&
             SystemModalContainerLayoutManager::IsModalBackground(*iter)) {
        ++iter;
      }
      // If the entire window list is modal background windows then stop.
      if (iter == src_container_children.end())
        break;
      ReparentWindow(*iter, dst_container);
    }
  }
}

// Creates a new window for use as a container.
WmWindow* CreateContainer(int window_id, const char* name, WmWindow* parent) {
  WmWindow* window = WmShell::Get()->NewWindow(ui::wm::WINDOW_TYPE_UNKNOWN,
                                               ui::LAYER_NOT_DRAWN);
  window->SetShellWindowId(window_id);
  window->SetName(name);
  parent->AddChild(window);
  if (window_id != kShellWindowId_UnparentedControlContainer)
    window->Show();
  return window;
}

}  // namespace

WmRootWindowController::WmRootWindowController(WmWindow* root)
    : root_(root), root_window_layout_manager_(nullptr) {}

WmRootWindowController::~WmRootWindowController() {
  if (animating_wallpaper_widget_controller_.get())
    animating_wallpaper_widget_controller_->StopAnimating();
}

void WmRootWindowController::SetWallpaperWidgetController(
    WallpaperWidgetController* controller) {
  wallpaper_widget_controller_.reset(controller);
}

void WmRootWindowController::SetAnimatingWallpaperWidgetController(
    AnimatingWallpaperWidgetController* controller) {
  if (animating_wallpaper_widget_controller_.get())
    animating_wallpaper_widget_controller_->StopAnimating();
  animating_wallpaper_widget_controller_.reset(controller);
}

wm::WorkspaceWindowState WmRootWindowController::GetWorkspaceWindowState() {
  return workspace_controller_ ? workspace_controller()->GetWindowState()
                               : wm::WORKSPACE_WINDOW_STATE_DEFAULT;
}

SystemModalContainerLayoutManager*
WmRootWindowController::GetSystemModalLayoutManager(WmWindow* window) {
  WmWindow* modal_container = nullptr;
  if (window) {
    WmWindow* window_container = wm::GetContainerForWindow(window);
    if (window_container &&
        window_container->GetShellWindowId() >=
            kShellWindowId_LockScreenContainer) {
      modal_container = GetContainer(kShellWindowId_LockSystemModalContainer);
    } else {
      modal_container = GetContainer(kShellWindowId_SystemModalContainer);
    }
  } else {
    int modal_window_id =
        WmShell::Get()->GetSessionStateDelegate()->IsUserSessionBlocked()
            ? kShellWindowId_LockSystemModalContainer
            : kShellWindowId_SystemModalContainer;
    modal_container = GetContainer(modal_window_id);
  }
  return modal_container ? static_cast<SystemModalContainerLayoutManager*>(
                               modal_container->GetLayoutManager())
                         : nullptr;
}

WmWindow* WmRootWindowController::GetContainer(int container_id) {
  return root_->GetChildByShellWindowId(container_id);
}

const WmWindow* WmRootWindowController::GetContainer(int container_id) const {
  return root_->GetChildByShellWindowId(container_id);
}

void WmRootWindowController::ShowContextMenu(
    const gfx::Point& location_in_screen,
    ui::MenuSourceType source_type) {
  ShellDelegate* delegate = WmShell::Get()->delegate();
  DCHECK(delegate);
  menu_model_.reset(delegate->CreateContextMenu(GetShelf(), nullptr));
  if (!menu_model_)
    return;

  menu_model_adapter_.reset(new views::MenuModelAdapter(
      menu_model_.get(), base::Bind(&WmRootWindowController::OnMenuClosed,
                                    base::Unretained(this))));

  // The wallpaper controller may not be set yet if the user clicked on the
  // status area before the initial animation completion. See crbug.com/222218
  if (!wallpaper_widget_controller())
    return;

  menu_runner_.reset(new views::MenuRunner(
      menu_model_adapter_->CreateMenu(),
      views::MenuRunner::CONTEXT_MENU | views::MenuRunner::ASYNC));
  ignore_result(
      menu_runner_->RunMenuAt(wallpaper_widget_controller()->widget(), nullptr,
                              gfx::Rect(location_in_screen, gfx::Size()),
                              views::MENU_ANCHOR_TOPLEFT, source_type));
}

void WmRootWindowController::OnInitialWallpaperAnimationStarted() {}

void WmRootWindowController::OnWallpaperAnimationFinished(
    views::Widget* widget) {
  WmShell::Get()->wallpaper_delegate()->OnWallpaperAnimationFinished();
  // Only removes old component when wallpaper animation finished. If we
  // remove the old one before the new wallpaper is done fading in there will
  // be a white flash during the animation.
  if (animating_wallpaper_widget_controller()) {
    WallpaperWidgetController* controller =
        animating_wallpaper_widget_controller()->GetController(true);
    DCHECK_EQ(controller->widget(), widget);
    // Release the old controller and close its wallpaper widget.
    SetWallpaperWidgetController(controller);
  }
}

void WmRootWindowController::MoveWindowsTo(WmWindow* dest) {
  // Clear the workspace controller, so it doesn't incorrectly update the shelf.
  DeleteWorkspaceController();
  ReparentAllWindows(GetWindow(), dest);
}

void WmRootWindowController::CreateContainers() {
  // These containers are just used by PowerButtonController to animate groups
  // of containers simultaneously without messing up the current transformations
  // on those containers. These are direct children of the root window; all of
  // the other containers are their children.

  // The wallpaper container is not part of the lock animation, so it is not
  // included in those animate groups. When the screen is locked, the wallpaper
  // is moved to the lock screen wallpaper container (and moved back on unlock).
  // Ensure that there's an opaque layer occluding the non-lock-screen layers.
  WmWindow* wallpaper_container = CreateContainer(
      kShellWindowId_WallpaperContainer, "WallpaperContainer", root_);
  wallpaper_container->SetChildWindowVisibilityChangesAnimated();

  WmWindow* non_lock_screen_containers =
      CreateContainer(kShellWindowId_NonLockScreenContainersContainer,
                      "NonLockScreenContainersContainer", root_);
  // Clip all windows inside this container, as half pixel of the window's
  // texture may become visible when the screen is scaled. crbug.com/368591.
  non_lock_screen_containers->SetMasksToBounds(true);

  WmWindow* lock_wallpaper_containers =
      CreateContainer(kShellWindowId_LockScreenWallpaperContainer,
                      "LockScreenWallpaperContainer", root_);
  lock_wallpaper_containers->SetChildWindowVisibilityChangesAnimated();

  WmWindow* lock_screen_containers =
      CreateContainer(kShellWindowId_LockScreenContainersContainer,
                      "LockScreenContainersContainer", root_);
  WmWindow* lock_screen_related_containers =
      CreateContainer(kShellWindowId_LockScreenRelatedContainersContainer,
                      "LockScreenRelatedContainersContainer", root_);

  CreateContainer(kShellWindowId_UnparentedControlContainer,
                  "UnparentedControlContainer", non_lock_screen_containers);

  WmWindow* default_container =
      CreateContainer(kShellWindowId_DefaultContainer, "DefaultContainer",
                      non_lock_screen_containers);
  default_container->SetChildWindowVisibilityChangesAnimated();
  default_container->SetSnapsChildrenToPhysicalPixelBoundary();
  default_container->SetBoundsInScreenBehaviorForChildren(
      WmWindow::BoundsInScreenBehavior::USE_SCREEN_COORDINATES);
  default_container->SetChildrenUseExtendedHitRegion();

  WmWindow* always_on_top_container =
      CreateContainer(kShellWindowId_AlwaysOnTopContainer,
                      "AlwaysOnTopContainer", non_lock_screen_containers);
  always_on_top_container->SetChildWindowVisibilityChangesAnimated();
  always_on_top_container->SetSnapsChildrenToPhysicalPixelBoundary();
  always_on_top_container->SetBoundsInScreenBehaviorForChildren(
      WmWindow::BoundsInScreenBehavior::USE_SCREEN_COORDINATES);

  WmWindow* docked_container =
      CreateContainer(kShellWindowId_DockedContainer, "DockedContainer",
                      non_lock_screen_containers);
  docked_container->SetChildWindowVisibilityChangesAnimated();
  docked_container->SetSnapsChildrenToPhysicalPixelBoundary();
  docked_container->SetBoundsInScreenBehaviorForChildren(
      WmWindow::BoundsInScreenBehavior::USE_SCREEN_COORDINATES);
  docked_container->SetChildrenUseExtendedHitRegion();

  WmWindow* shelf_container =
      CreateContainer(kShellWindowId_ShelfContainer, "ShelfContainer",
                      non_lock_screen_containers);
  shelf_container->SetSnapsChildrenToPhysicalPixelBoundary();
  shelf_container->SetBoundsInScreenBehaviorForChildren(
      WmWindow::BoundsInScreenBehavior::USE_SCREEN_COORDINATES);
  shelf_container->SetLockedToRoot(true);

  WmWindow* panel_container =
      CreateContainer(kShellWindowId_PanelContainer, "PanelContainer",
                      non_lock_screen_containers);
  panel_container->SetSnapsChildrenToPhysicalPixelBoundary();
  panel_container->SetBoundsInScreenBehaviorForChildren(
      WmWindow::BoundsInScreenBehavior::USE_SCREEN_COORDINATES);

  WmWindow* shelf_bubble_container =
      CreateContainer(kShellWindowId_ShelfBubbleContainer,
                      "ShelfBubbleContainer", non_lock_screen_containers);
  shelf_bubble_container->SetSnapsChildrenToPhysicalPixelBoundary();
  shelf_bubble_container->SetBoundsInScreenBehaviorForChildren(
      WmWindow::BoundsInScreenBehavior::USE_SCREEN_COORDINATES);
  shelf_bubble_container->SetLockedToRoot(true);

  WmWindow* app_list_container =
      CreateContainer(kShellWindowId_AppListContainer, "AppListContainer",
                      non_lock_screen_containers);
  app_list_container->SetSnapsChildrenToPhysicalPixelBoundary();
  app_list_container->SetBoundsInScreenBehaviorForChildren(
      WmWindow::BoundsInScreenBehavior::USE_SCREEN_COORDINATES);

  WmWindow* modal_container =
      CreateContainer(kShellWindowId_SystemModalContainer,
                      "SystemModalContainer", non_lock_screen_containers);
  modal_container->SetSnapsChildrenToPhysicalPixelBoundary();
  modal_container->SetChildWindowVisibilityChangesAnimated();
  modal_container->SetBoundsInScreenBehaviorForChildren(
      WmWindow::BoundsInScreenBehavior::USE_SCREEN_COORDINATES);
  modal_container->SetChildrenUseExtendedHitRegion();

  // TODO(beng): Figure out if we can make this use
  // SystemModalContainerEventFilter instead of stops_event_propagation.
  WmWindow* lock_container =
      CreateContainer(kShellWindowId_LockScreenContainer, "LockScreenContainer",
                      lock_screen_containers);
  lock_container->SetSnapsChildrenToPhysicalPixelBoundary();
  lock_container->SetBoundsInScreenBehaviorForChildren(
      WmWindow::BoundsInScreenBehavior::USE_SCREEN_COORDINATES);
  // TODO(beng): stopsevents

  WmWindow* lock_modal_container =
      CreateContainer(kShellWindowId_LockSystemModalContainer,
                      "LockSystemModalContainer", lock_screen_containers);
  lock_modal_container->SetSnapsChildrenToPhysicalPixelBoundary();
  lock_modal_container->SetChildWindowVisibilityChangesAnimated();
  lock_modal_container->SetBoundsInScreenBehaviorForChildren(
      WmWindow::BoundsInScreenBehavior::USE_SCREEN_COORDINATES);
  lock_modal_container->SetChildrenUseExtendedHitRegion();

  WmWindow* status_container =
      CreateContainer(kShellWindowId_StatusContainer, "StatusContainer",
                      lock_screen_related_containers);
  status_container->SetSnapsChildrenToPhysicalPixelBoundary();
  status_container->SetBoundsInScreenBehaviorForChildren(
      WmWindow::BoundsInScreenBehavior::USE_SCREEN_COORDINATES);
  status_container->SetLockedToRoot(true);

  WmWindow* settings_bubble_container =
      CreateContainer(kShellWindowId_SettingBubbleContainer,
                      "SettingBubbleContainer", lock_screen_related_containers);
  settings_bubble_container->SetChildWindowVisibilityChangesAnimated();
  settings_bubble_container->SetSnapsChildrenToPhysicalPixelBoundary();
  settings_bubble_container->SetBoundsInScreenBehaviorForChildren(
      WmWindow::BoundsInScreenBehavior::USE_SCREEN_COORDINATES);
  settings_bubble_container->SetLockedToRoot(true);

  WmWindow* virtual_keyboard_parent_container = CreateContainer(
      kShellWindowId_ImeWindowParentContainer, "VirtualKeyboardParentContainer",
      lock_screen_related_containers);
  virtual_keyboard_parent_container->SetSnapsChildrenToPhysicalPixelBoundary();
  virtual_keyboard_parent_container->SetBoundsInScreenBehaviorForChildren(
      WmWindow::BoundsInScreenBehavior::USE_SCREEN_COORDINATES);

  WmWindow* menu_container =
      CreateContainer(kShellWindowId_MenuContainer, "MenuContainer",
                      lock_screen_related_containers);
  menu_container->SetChildWindowVisibilityChangesAnimated();
  menu_container->SetSnapsChildrenToPhysicalPixelBoundary();
  menu_container->SetBoundsInScreenBehaviorForChildren(
      WmWindow::BoundsInScreenBehavior::USE_SCREEN_COORDINATES);

  WmWindow* drag_drop_container = CreateContainer(
      kShellWindowId_DragImageAndTooltipContainer,
      "DragImageAndTooltipContainer", lock_screen_related_containers);
  drag_drop_container->SetChildWindowVisibilityChangesAnimated();
  drag_drop_container->SetSnapsChildrenToPhysicalPixelBoundary();
  drag_drop_container->SetBoundsInScreenBehaviorForChildren(
      WmWindow::BoundsInScreenBehavior::USE_SCREEN_COORDINATES);

  WmWindow* overlay_container =
      CreateContainer(kShellWindowId_OverlayContainer, "OverlayContainer",
                      lock_screen_related_containers);
  overlay_container->SetSnapsChildrenToPhysicalPixelBoundary();
  overlay_container->SetBoundsInScreenBehaviorForChildren(
      WmWindow::BoundsInScreenBehavior::USE_SCREEN_COORDINATES);

#if defined(OS_CHROMEOS)
  WmWindow* mouse_cursor_container = CreateContainer(
      kShellWindowId_MouseCursorContainer, "MouseCursorContainer", root_);
  mouse_cursor_container->SetBoundsInScreenBehaviorForChildren(
      WmWindow::BoundsInScreenBehavior::USE_SCREEN_COORDINATES);
#endif

  CreateContainer(kShellWindowId_PowerButtonAnimationContainer,
                  "PowerButtonAnimationContainer", root_);
}

void WmRootWindowController::CreateLayoutManagers() {
  root_window_layout_manager_ = new wm::RootWindowLayoutManager(root_);
  root_->SetLayoutManager(base::WrapUnique(root_window_layout_manager_));

  WmWindow* default_container = GetContainer(kShellWindowId_DefaultContainer);
  workspace_controller_.reset(new WorkspaceController(default_container));

  WmWindow* modal_container = GetContainer(kShellWindowId_SystemModalContainer);
  DCHECK(modal_container);
  modal_container->SetLayoutManager(
      base::MakeUnique<SystemModalContainerLayoutManager>(modal_container));

  WmWindow* lock_modal_container =
      GetContainer(kShellWindowId_LockSystemModalContainer);
  DCHECK(lock_modal_container);
  lock_modal_container->SetLayoutManager(
      base::MakeUnique<SystemModalContainerLayoutManager>(
          lock_modal_container));
}

void WmRootWindowController::DeleteWorkspaceController() {
  workspace_controller_.reset();
}

void WmRootWindowController::OnMenuClosed() {
  menu_runner_.reset();
  menu_model_adapter_.reset();
  menu_model_.reset();
  GetShelf()->UpdateVisibilityState();
}

}  // namespace ash
