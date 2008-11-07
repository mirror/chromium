// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef ENABLE_BACKGROUND_TASK

#include "chrome/browser/notifications/user_activity.h"

#include "base/gfx/rect.h"
#include "base/logging.h"
#include "base/timer.h"

#if defined(OS_WIN)
#include <windows.h>
#endif  // OS_WIN

// Constants.
static const size_t kDefaultUserIdleThresholdSec = 5 * 60;    // 5m
static const size_t kDefaultUserBusyThresholdSec = 30;        // 30s

// In Windows platform, the balloon will reappear shortly after the use
// enters full-screen mode. To work around this, we use a much shorter 
// interval.
#if defined(OS_WIN)
static const int64 kUserActivityCheckIntervalMs = 500;    // 0.5s
#else
static const int64 kUserActivityCheckIntervalMs = 1000;   // 1s
#endif  // OS_WIN

UserActivityMonitor::UserActivityMonitor()
    : user_mode_(USER_MODE_UNKNOWN) {
  timer_.Start(base::TimeDelta::FromMilliseconds(kUserActivityCheckIntervalMs),
               this,
               &UserActivityMonitor::CheckNow);
}

UserActivityMonitor::~UserActivityMonitor() {
  timer_.Stop();
}

void UserActivityMonitor::AddObserver(UserActivityObserver *observer) {
  DCHECK(observer);
  observers_.push_back(observer);
}

bool UserActivityMonitor::IsUserIdle() {
  uint32 idle_threshold_sec = kDefaultUserIdleThresholdSec;
  uint32 power_off_sec = GetMonitorPowerOffTimeSec();
  if (power_off_sec != 0 && power_off_sec < idle_threshold_sec) {
    idle_threshold_sec = power_off_sec;
  }

  return GetUserIdleTimeMs() > idle_threshold_sec * 1000;
}

bool UserActivityMonitor::IsUserBusy() {
  return GetUserIdleTimeMs() < kDefaultUserBusyThresholdSec * 1000;
}

bool UserActivityMonitor::IsUserAway() {
  return IsScreensaverRunning() || IsWorkstationLocked();
}

void UserActivityMonitor::CheckNow() {
  UserMode previous_user_mode = user_mode_;
  user_mode_ = GetUserActivity();
  if (previous_user_mode != user_mode_) {
    for (size_t i = 0; i < observers_.size(); ++i) {
      observers_[i]->OnUserActivityChange();
    }
  }
}

UserMode UserActivityMonitor::GetUserActivity() {
  // Use platform specific way of detecting user mode.
  UserMode user_mode = PlatformDetectUserMode();
  if (user_mode != USER_MODE_UNKNOWN) {
    return user_mode;
  }

  // Check if the user is away.
  // Note that this check should be done before fullscreen check because
  // when screensaver is running, it could be in fullscreen window.
  if (IsUserAway()) {
    return USER_AWAY_MODE;
  }

  // Check if the user is in fullscreen mode.
  if (IsFullScreenMode()) {
    return USER_PRESENTATION_MODE;
  }

  // Check if the user is idling.
  return IsUserIdle() ? USER_IDLE_MODE : USER_NORMAL_MODE;
}


// Win32 Implementations.
#if defined(OS_WIN)

class Win32UserActivityMonitor : public UserActivityMonitor {
 public:
  Win32UserActivityMonitor();
  virtual ~Win32UserActivityMonitor();

 protected:
  virtual UserMode PlatformDetectUserMode();
  virtual uint32 GetMonitorPowerOffTimeSec();
  virtual uint32 GetUserIdleTimeMs();
  virtual bool IsScreensaverRunning();
  virtual bool IsWorkstationLocked() ;
  virtual bool IsFullScreenMode();

 private:
  DISALLOW_COPY_AND_ASSIGN(Win32UserActivityMonitor);
};

UserActivityMonitor *UserActivityMonitor::Create() {
  return new Win32UserActivityMonitor();
}

Win32UserActivityMonitor::Win32UserActivityMonitor() {
}

Win32UserActivityMonitor::~Win32UserActivityMonitor() {
}

UserMode Win32UserActivityMonitor::PlatformDetectUserMode() {
  // TODO (jianli): Calls Vista function.
  return USER_MODE_UNKNOWN;
}

uint32 Win32UserActivityMonitor::GetMonitorPowerOffTimeSec() {
  BOOL power_off_enabled = FALSE;
  if (::SystemParametersInfo(SPI_GETPOWEROFFACTIVE, 0, &power_off_enabled, 0) &&
      power_off_enabled) {
    int power_off_sec = 0;
    if (::SystemParametersInfo(SPI_GETPOWEROFFTIMEOUT, 0, &power_off_sec, 0)) {
      return power_off_sec;
    }
  }
  return ULONG_MAX;
}

uint32 Win32UserActivityMonitor::GetUserIdleTimeMs() {
  LASTINPUTINFO last_input_info = {0};
  last_input_info.cbSize = sizeof(LASTINPUTINFO);
  if (::GetLastInputInfo(&last_input_info)) {
    return ::GetTickCount() - last_input_info.dwTime;
  }
  return 0;
}

bool Win32UserActivityMonitor::IsScreensaverRunning() {
  DWORD result = 0;
  if (::SystemParametersInfo(SPI_GETSCREENSAVERRUNNING, 0, &result, 0)) {
    return result != FALSE;
  }
  return false;
}

bool Win32UserActivityMonitor::IsWorkstationLocked() {
  bool is_locked = true;
  HDESK input_desk = ::OpenInputDesktop(0, 0, GENERIC_READ);
  if (input_desk)  {
    wchar_t name[256] = {0};
    DWORD needed = 0;
    if (::GetUserObjectInformation(input_desk,
                                   UOI_NAME,
                                   name,
                                   sizeof(name),
                                   &needed)) {
      is_locked = lstrcmpi(name, L"default") != 0;
    }
    ::CloseDesktop(input_desk);
  }
  return is_locked;
}

bool Win32UserActivityMonitor::IsFullScreenMode() {
  // Check if in full screen window mode.
  // 1) Get the window from any point lies at the main screen where we show
  //    show the notifications.
  // 2) Its window rect is at least as large as the monitor resolution.
  //    Typically for any maximized window the full rect is only as large or
  //    slightly larger than the monitor work area, not the full rect.
  //    Note that this is broken if a maximized window is displayed in either
  //    of the following situation:
  //    * The user hides the taskbar, sidebar and all other top-most bars.
  //    * The user is putting the window in the secondary monitor.
  // 3) Its style should not have WS_DLGFRAME and WS_THICKFRAME;
  //    and its extended style should not have WS_EX_WINDOWEDGE and
  //    WS_EX_TOOLWINDOW.
  POINT point = {0};
  RECT work_area = {0};
  if (::SystemParametersInfo(SPI_GETWORKAREA, 0, &work_area, 0)) {
    point.x = work_area.left;
    point.y = work_area.left;
  }
  HWND wnd = ::GetAncestor(::WindowFromPoint(point), GA_ROOT);
  if (wnd != ::GetDesktopWindow()) {
    RECT wnd_rc = {0};
    if (::GetWindowRect(wnd, &wnd_rc)) {
      HMONITOR mon = ::MonitorFromRect(&wnd_rc, MONITOR_DEFAULTTONULL);
      if (mon) {
        MONITORINFO mi = { sizeof(mi) };
        ::GetMonitorInfo(mon, &mi);
        ::IntersectRect(&wnd_rc, &wnd_rc, &mi.rcMonitor);
        if (::EqualRect(&wnd_rc, &mi.rcMonitor)) {
          LONG style = ::GetWindowLong(wnd, GWL_STYLE);
          LONG exstyle = ::GetWindowLong(wnd, GWL_EXSTYLE);
          if (!((style & (WS_DLGFRAME | WS_THICKFRAME)) ||
                (exstyle & (WS_EX_WINDOWEDGE | WS_EX_TOOLWINDOW)))) {
            return true;
          }
        }
      }
    }
  }

  // Check if in full screen console mode.
  // We do this by attaching the current process to the console of the
  // foreground window and then checking if it is in full screen mode.
  DWORD pid = 0;
  ::GetWindowThreadProcessId(::GetForegroundWindow(), &pid);
  if (pid) {
    if (::AttachConsole(pid)) {
      DWORD mode_flags = 0;
      ::GetConsoleDisplayMode(&mode_flags);
      ::FreeConsole();
      if (mode_flags & (CONSOLE_FULLSCREEN | CONSOLE_FULLSCREEN_HARDWARE)) {
        return true;
      }
    }
  }

  return false;
}

#endif  // OS_WIN

#endif  // ENABLE_BACKGROUND_TASK
