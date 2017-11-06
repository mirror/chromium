// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_WM_HELPER_H_
#define COMPONENTS_EXO_WM_HELPER_H_

#include "ash/host/window_tree_host_manager.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "ui/aura/client/drag_drop_delegate.h"
#include "ui/base/cursor/cursor.h"
#include "ui/compositor/compositor_vsync_manager.h"

namespace ash {
class TabletModeObserver;
}

namespace aura {
class Window;
namespace client {
class ClientClientObserver;
class FocusChangeObserver;
}
}

namespace wm {
class ActivationChangeObserver;
}

namespace display {
class Display;
class ManagedDisplayInfo;
}

namespace ui {
class EventHandler;
class DropTargetEvent;
}

namespace

namespace exo {

// A helper class for accessing WindowManager related features.
class WMHelper : public aura::client::DragDropDelegate {
 public:
  class DragDropObserver {
   public:
    virtual void OnDragEntered(const ui::DropTargetEvent& event) = 0;
    virtual int OnDragUpdated(const ui::DropTargetEvent& event) = 0;
    virtual void OnDragExited() = 0;
    virtual int OnPerformDrop(const ui::DropTargetEvent& event) = 0;

   protected:
    virtual ~DragDropObserver() {}
  };

  ~WMHelper() override;

  static void SetInstance(WMHelper* helper);
  static WMHelper* GetInstance();
  static bool HasInstance();

  void AddActivationObserver(wm::ActivationChangeObserver* observer);
  void RemoveActivationObserver(wm::ActivationChangeObserver* observer);
  void AddFocusObserver(aura::client::FocusChangeObserver* observer);
  void RemoveFocusObserver(aura::client::FocusChangeObserver* observer);
  void AddCursorObserver(aura::client::CursorClientObserver* observer);
  void RemoveCursorObserver(aura::client::CursorClientObserver* observer);
  void AddTabletModeObserver(ash::TabletModeObserver* observer);
  void RemoveTabletModeObserver(ash::TabletModeObserver* observer);
  void AddInputDeviceEventObserver(ui::InputDeviceEventObserver* observer);
  void RemoveInputDeviceEventObserver(ui::InputDeviceEventObserver* observer);

  void AddDisplayConfigurationObserver(ash::WindowTreeHostManager::Observer* observer);
  void RemoveDisplayConfigurationObserver(
      ash::WindowTreeHostManager::Observer* observer);
  void AddDragDropObserver(DragDropObserver* observer);
  void RemoveDragDropObserver(DragDropObserver* observer);
  void SetDragDropDelegate(aura::Window*);
  void ResetDragDropDelegate(aura::Window*);
  void AddVSyncObserver(ui::CompositorVSyncManager::Observer* observer);
  void RemoveVSyncObserver(ui::CompositorVSyncManager::Observer* observer);

  virtual const display::ManagedDisplayInfo& GetDisplayInfo(
      int64_t display_id) const = 0;
  virtual aura::Window* GetPrimaryDisplayContainer(int container_id) = 0;
  virtual aura::Window* GetActiveWindow() const = 0;
  virtual aura::Window* GetFocusedWindow() const = 0;
  virtual ui::CursorSize GetCursorSize() const = 0;
  virtual const display::Display& GetCursorDisplay() const = 0;
  virtual void AddPreTargetHandler(ui::EventHandler* handler) = 0;
  virtual void PrependPreTargetHandler(ui::EventHandler* handler) = 0;
  virtual void RemovePreTargetHandler(ui::EventHandler* handler) = 0;
  virtual void AddPostTargetHandler(ui::EventHandler* handler) = 0;
  virtual void RemovePostTargetHandler(ui::EventHandler* handler) = 0;
  virtual bool IsTabletModeWindowManagerEnabled() const = 0;
  virtual double GetDefaultDeviceScaleFactor() const = 0;
  virtual bool AreVerifiedSyncTokensNeeded() const = 0;

  // Overridden from aura::client::DragDropDelegate:
  void OnDragEntered(const ui::DropTargetEvent& event) override;
  int OnDragUpdated(const ui::DropTargetEvent& event) override;
  void OnDragExited() override;
  int OnPerformDrop(const ui::DropTargetEvent& event) override;

 protected:
  WMHelper();

  void NotifyWindowActivated(aura::Window* gained_active,
                             aura::Window* lost_active);
  void NotifyWindowFocused(aura::Window* gained_focus,
                           aura::Window* lost_focus);
  void NotifyCursorVisibilityChanged(bool is_visible);
  void NotifyCursorSizeChanged(ui::CursorSize cursor_size);
  void NotifyCursorDisplayChanged(const display::Display& display);
  void NotifyTabletModeStarted();
  void NotifyTabletModeEnding();
  void NotifyTabletModeEnded();
  void NotifyKeyboardDeviceConfigurationChanged();
  void NotifyDisplayConfigurationChanged();
  void NotifyUpdateVSyncParameters(base::TimeTicks timebase,
                                   base::TimeDelta interval);

 private:
  base::ObserverList<ActivationObserver> activation_observers_;
  base::ObserverList<FocusObserver> focus_observers_;
  base::ObserverList<CursorObserver> cursor_observers_;
  base::ObserverList<TabletModeObserver> tablet_mode_observers_;
  base::ObserverList<InputDeviceEventObserver> input_device_event_observers_;
  base::ObserverList<DisplayConfigurationObserver> display_config_observers_;
  base::ObserverList<DragDropObserver> drag_drop_observers_;
  base::ObserverList<VSyncObserver> vsync_observers_;

  // The most recently cached VSync parameters, sent to observers on addition.
  base::TimeTicks vsync_timebase_;
  base::TimeDelta vsync_interval_;

  DISALLOW_COPY_AND_ASSIGN(WMHelper);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_WM_HELPER_H_
