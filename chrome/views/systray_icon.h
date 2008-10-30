// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_VIEWS_SYSTRAY_ICON_H_
#define CHROME_VIEWS_SYSTRAY_ICON_H_

#ifdef ENABLE_BACKGROUND_TASK
#include <vector>
#include <windows.h>
#include "base/time.h"
#include "base/timer.h"

namespace views {

class SystrayIcon : public CWindowImpl<SystrayIcon> {
 public:
  // Implemented by an object wishing to know about clicks, double clicks, etc.
  class Listener {
   public:
    virtual void LeftClick() {}
    virtual void RightClick() {}
    virtual void LeftDoubleClick() {}
  };

  static SystrayIcon* CreateInstance(Listener* listener);
  ~SystrayIcon();

  // Show or hide the icon.
  bool Show(bool to_show);

  // Sets the icon to display in the task bar.
  // It is copied so the passed in icon is still owned by the caller.
  bool SetIcon(HICON icon);
  bool SetTip(const std::wstring& tip);

  // Animate the icon.
  // All passed in icons are copied.
  bool StartAnimation(HICON* animation_icons,
                      int animation_icon_count,
                      const TimeDelta& frame_time,
                      int repetitions);
  bool StopAnimation();

  static const int kWmSystrayNotify = WM_USER + 0;

  BEGIN_MSG_MAP(NotifierDummyWindow)
    MESSAGE_HANDLER(kWmSystrayNotify, OnSystrayMessage)
    MESSAGE_HANDLER(taskbar_created_msg_, OnTaskbarCreated)
  END_MSG_MAP();

 private:
  SystrayIcon(Listener* listener);
  LRESULT OnSystrayMessage(UINT msg, WPARAM wparam, LPARAM lparam,
                           BOOL& handled);
  LRESULT OnTaskbarCreated(UINT msg, WPARAM wparam, LPARAM lparam,
                           BOOL& handled);

  bool UpdateSystray(DWORD message, DWORD flags);

  void AnimationTimerCallback();

  HICON icon_;
  std::wstring tip_;
  bool showing_;
  base::RepeatingTimer<SystrayIcon> animation_timer_;
  TimeTicks animation_start_;
  TimeTicks animation_end_;
  TimeDelta frame_time_;
  std::vector<HICON> animation_icons_;
  uint32 taskbar_created_msg_;
  Listener* listener_;
  DISALLOW_COPY_AND_ASSIGN(SystrayIcon);
};

}  // namespace views

#endif  // ENABLE_BACKGROUND_TASK
#endif  // CHROME_VIEWS_SYSTRAY_ICON_H_
