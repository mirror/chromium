// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_SESSION_TRAY_OFF_HOURS_SESSION_LIMIT_H_
#define ASH_SYSTEM_SESSION_TRAY_OFF_HOURS_SESSION_LIMIT_H_

#include <memory>

#include "ash/session/session_observer.h"
#include "ash/system/tray/system_tray_item.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "base/timer/timer.h"

namespace ash {

class LabelTrayView;

// TODO(yakovleva): description
class ASH_EXPORT TrayOffHoursSessionLimit : public SystemTrayItem,
                                            public SessionObserver {
 public:
  enum OffHoursMode { OFF_HOURS_OFF, OFF_HOURS_ON, OFF_HOURS_EXPIRING_SOON };

  explicit TrayOffHoursSessionLimit(SystemTray* system_tray);
  ~TrayOffHoursSessionLimit() override;

  // SystemTrayItem:
  views::View* CreateDefaultView(LoginStatus status) override;
  void OnDefaultViewDestroyed() override;

  // SessionObserver:
  void OnSessionStateChanged(session_manager::SessionState state) override;

  // OFF HOURS
  void OnOffHoursModeChanged() override;

 private:
  static const char kNotificationId[];

  // Update state, notification and tray bubble view.  Called by the
  // RepeatingTimer in regular intervals and also by OnSessionChanged().
  void Update();

  // Recalculate |off_hours_mode_| and |remaining_session_time_|.
  void UpdateState();

  void UpdateNotification();
  void UpdateTrayBubbleView() const;

  // These require that the state has been updated before.
  base::string16 ComposeNotificationMessage() const;
  base::string16 ComposeTrayBubbleMessage() const;

  base::TimeDelta remaining_session_time_;

  OffHoursMode off_hours_mode_;       // Current state.
  OffHoursMode last_off_hours_mode_;  // State of last notification update.

  LabelTrayView* tray_bubble_view_;
  std::unique_ptr<base::RepeatingTimer> timer_;

  DISALLOW_COPY_AND_ASSIGN(TrayOffHoursSessionLimit);
};

}  // namespace ash

#endif  // ASH_SYSTEM_SESSION_TRAY_OFF_HOURS_SESSION_LIMIT_H_