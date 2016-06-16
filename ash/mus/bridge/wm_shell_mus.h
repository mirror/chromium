// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_MUS_BRIDGE_WM_SHELL_MUS_H_
#define ASH_MUS_BRIDGE_WM_SHELL_MUS_H_

#include <stdint.h>

#include <vector>

#include "ash/common/wm_shell.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "components/mus/public/cpp/window_tree_client_observer.h"

namespace mus {
class WindowTreeClient;
}

namespace ash {

class WmShellCommon;

namespace mus {

class WmRootWindowControllerMus;
class WmWindowMus;

// WmShell implementation for mus.
class WmShellMus : public WmShell, public ::mus::WindowTreeClientObserver {
 public:
  explicit WmShellMus(::mus::WindowTreeClient* client);
  ~WmShellMus() override;

  static WmShellMus* Get();

  void AddRootWindowController(WmRootWindowControllerMus* controller);
  void RemoveRootWindowController(WmRootWindowControllerMus* controller);

  // Returns the ancestor of |window| (including |window|) that is considered
  // toplevel. |window| may be null.
  static WmWindowMus* GetToplevelAncestor(::mus::Window* window);

  WmRootWindowControllerMus* GetRootWindowControllerWithDisplayId(int64_t id);

  // WmShell:
  MruWindowTracker* GetMruWindowTracker() override;
  WmWindow* NewContainerWindow() override;
  WmWindow* GetFocusedWindow() override;
  WmWindow* GetActiveWindow() override;
  WmWindow* GetPrimaryRootWindow() override;
  WmWindow* GetRootWindowForDisplayId(int64_t display_id) override;
  WmWindow* GetRootWindowForNewWindows() override;
  bool IsForceMaximizeOnFirstRun() override;
  bool CanShowWindowForUser(WmWindow* window) override;
  void LockCursor() override;
  void UnlockCursor() override;
  std::vector<WmWindow*> GetAllRootWindows() override;
  void RecordUserMetricsAction(UserMetricsAction action) override;
  std::unique_ptr<WindowResizer> CreateDragWindowResizer(
      std::unique_ptr<WindowResizer> next_window_resizer,
      wm::WindowState* window_state) override;
  void OnOverviewModeStarting() override;
  void OnOverviewModeEnded() override;
  bool IsOverviewModeSelecting() override;
  bool IsOverviewModeRestoringMinimizedWindows() override;
  AccessibilityDelegate* GetAccessibilityDelegate() override;
  SessionStateDelegate* GetSessionStateDelegate() override;
  void AddActivationObserver(WmActivationObserver* observer) override;
  void RemoveActivationObserver(WmActivationObserver* observer) override;
  void AddDisplayObserver(WmDisplayObserver* observer) override;
  void RemoveDisplayObserver(WmDisplayObserver* observer) override;
  void AddShellObserver(ShellObserver* observer) override;
  void RemoveShellObserver(ShellObserver* observer) override;
  void NotifyPinnedStateChanged(WmWindow* pinned_window) override;

 private:
  // Returns true if |window| is a window that can have active children.
  static bool IsActivationParent(::mus::Window* window);

  void RemoveClientObserver();

  // ::mus::WindowTreeClientObserver:
  void OnWindowTreeFocusChanged(::mus::Window* gained_focus,
                                ::mus::Window* lost_focus) override;
  void OnWillDestroyClient(::mus::WindowTreeClient* client) override;

  ::mus::WindowTreeClient* client_;

  std::unique_ptr<WmShellCommon> wm_shell_common_;

  std::vector<WmRootWindowControllerMus*> root_window_controllers_;

  std::unique_ptr<SessionStateDelegate> session_state_delegate_;

  std::unique_ptr<AccessibilityDelegate> accessibility_delegate_;

  base::ObserverList<WmActivationObserver> activation_observers_;

  DISALLOW_COPY_AND_ASSIGN(WmShellMus);
};

}  // namespace mus
}  // namespace ash

#endif  // ASH_MUS_BRIDGE_WM_SHELL_MUS_H_
