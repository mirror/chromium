// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef ENABLE_BACKGROUND_TASK
#include "chrome/views/systray_icon.h"

#include <ShellAPI.h>

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/string_util.h"
#include "base/timer.h"

namespace views {
SystrayIcon::SystrayIcon(Listener* listener)
    : showing_(false),
      listener_(listener) {
  DCHECK(listener);
}

SystrayIcon::~SystrayIcon() {
  // Stop animations, clean up icons, and detach from the systray.
  SetIcon(NULL);
  DCHECK(icon_ == NULL && animation_icons_.size() == 0);
}

SystrayIcon* SystrayIcon::CreateInstance(Listener* listener) {
  SystrayIcon* systray = new SystrayIcon(listener);
  // Get the message the explorer will send when crashing and restarting.
  // Note: Do this before creating the window or else the message map maps
  // a random message to OnTaskbarCreated.
  systray->taskbar_created_msg_ = ::RegisterWindowMessage(L"TaskbarCreated");
  DCHECK(systray->taskbar_created_msg_);

  if (!systray->Create(NULL, 0, L"Chrome_Systray", WS_POPUP)) {
    delete systray;
    return NULL;
  }
  return systray;
}

bool SystrayIcon::Show(bool to_show) {
  if (to_show && !showing_) {
    if (!UpdateSystray(NIM_ADD, NIF_ICON | NIF_MESSAGE | NIF_TIP)) {
      return false;
    }
  } else if (!to_show && showing_) {
    // When the icon is hidden, any on going animations
    // should be ended.
    StopAnimation();
    if (!UpdateSystray(NIM_DELETE, 0)) {
      return false;
    }
  }

  showing_ = to_show;
  return true;
}

bool SystrayIcon::SetIcon(HICON icon) {
  if (icon_) {
    ::DestroyIcon(icon_);
    icon_ = NULL;
  }
  if (!icon) {
    Show(false);
    StopAnimation();
    return true;
  }

  icon_ = ::CopyIcon(icon);
  if (!icon_) {
    return false;
  }

  if (!showing_) {
    return true;
  }

  return UpdateSystray(NIM_MODIFY, NIF_ICON);
}

bool SystrayIcon::StartAnimation(HICON* animation_icons,
                                 int animation_icon_count,
                                 const base::TimeDelta& frame_time,
                                 int repetitions) {
  DCHECK(animation_icons);
  DCHECK(animation_icon_count > 0);
  DCHECK(frame_time.InMilliseconds() > 0);
  DCHECK(icon_);

  StopAnimation();

  for (int i = 0; i < animation_icon_count; ++i) {
    HICON icon = ::CopyIcon(animation_icons[i]);
    if (!icon) {
      // Clean up any icons that have been already added in this loop.
      StopAnimation();
      return false;
    }
    animation_icons_.push_back(icon);
  }

  frame_time_ = frame_time;
  animation_start_ = base::TimeTicks::Now();
  animation_end_ = animation_start_ +
      frame_time_ * animation_icons_.size() * repetitions;
  UpdateSystray(NIM_MODIFY, NIF_ICON);

  animation_timer_.Start(frame_time_, this,
                         &SystrayIcon::AnimationTimerCallback);
  return true;
}

bool SystrayIcon::StopAnimation() {
  if (animation_icons_.empty()) {
    return true;
  }
  animation_timer_.Stop();
  for (size_t i = 1; i < animation_icons_.size(); ++i) {
    ::DestroyIcon(animation_icons_[i]);
  }
  animation_icons_.clear();

  // Restore the main icon
  UpdateSystray(NIM_MODIFY, NIF_ICON);
  return true;
}

// Timer callback when a blinking timer is filed
void SystrayIcon::AnimationTimerCallback() {
  UpdateSystray(NIM_MODIFY, NIF_ICON);
}

// Set the tip
bool SystrayIcon::SetTip(const std::wstring& tip) {
  tip_ = tip;

  if (!showing_) {
    return true;
  }

  return UpdateSystray(NIM_MODIFY, NIF_ICON | NIF_MESSAGE | NIF_TIP);
}

bool SystrayIcon::UpdateSystray(DWORD message, DWORD flags) {
  NOTIFYICONDATA nid = { 0 };
  nid.cbSize = sizeof(nid);
  nid.uFlags = flags;
  nid.hWnd = m_hWnd;
  nid.hIcon = icon_;
  nid.uCallbackMessage = kWmSystrayNotify;

  base::TimeTicks now = base::TimeTicks::Now();
  if (animation_icons_.size() > 0) {
    if (now >= animation_end_) {
      StopAnimation();
    } else {
      int64 frame_count = (now - animation_start_) / frame_time_;
      int animation_index =
          static_cast<int>(frame_count % animation_icons_.size());
      nid.hIcon = animation_icons_[animation_index];
    }
  }

  if (!tip_.empty()) {
    base::wcslcpy(nid.szTip, tip_.c_str(), arraysize(nid.szTip));
  }

  // TODO(levin): Consider adding retries for this api call
  // (in an async fashion) to handle random Shell_Notify failures.
  // (see http://msdn.microsoft.com/en-us/library/bb762159(VS.85).aspx).
  if (!::Shell_NotifyIcon(message, &nid)) {
    DLOG(ERROR) << "Calling Shell_NotifyIcon failed.";
    return false;
  }
  return true;
}

LRESULT SystrayIcon::OnSystrayMessage(UINT message,
                                      WPARAM wparam,
                                      LPARAM lparam,
                                      BOOL& handled) {
  handled = FALSE;
  switch (lparam)  {
    case WM_LBUTTONDOWN:
      listener_->LeftClick();
      handled = TRUE;
      break;

    case WM_RBUTTONUP:
      listener_->RightClick();
      handled = TRUE;
      break;

    case WM_LBUTTONDBLCLK:
      listener_->LeftDoubleClick();
      handled = TRUE;
      break;
  }
  return 0;
}

LRESULT SystrayIcon::OnTaskbarCreated(UINT message,
                                      WPARAM wparam,
                                      LPARAM lparam,
                                      BOOL& handled) {
  handled = TRUE;
  if (showing_) {
    // Display on the newly created taskbar.
    showing_ = false;
    Show(true);
  }
  return 0;
}

}  // namespace views
#endif  // ENABLE_BACKGROUND_TASK
