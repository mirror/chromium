// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_RELAUNCH_NOTIFICATION_RELAUNCH_RECOMMENDED_BUBBLE_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_RELAUNCH_NOTIFICATION_RELAUNCH_RECOMMENDED_BUBBLE_VIEW_H_

#include "base/macros.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "chrome/browser/ui/views/location_bar/location_bar_bubble_delegate_view.h"

class Browser;

namespace views {
class View;
class Widget;
}  // namespace views

class RelaunchRecommendedBubbleView : public LocationBarBubbleDelegateView {
 public:
  static views::Widget* ShowBubble(Browser* browser,
                                   base::TimeTicks detection_time);
  ~RelaunchRecommendedBubbleView() override;

  // views::DialogDelegate:
  bool Accept() override;
  bool Close() override;

  // ui::DialogModel:
  int GetDialogButtons() const override;
  base::string16 GetDialogButtonLabel(ui::DialogButton button) const override;

  // views::WidgetDelegate:
  base::string16 GetWindowTitle() const override;
  bool ShouldShowCloseButton() const override;
  gfx::ImageSkia GetWindowIcon() override;
  bool ShouldShowWindowIcon() const override;

 protected:
  // views::BubbleDialogDelegateView:
  void Init() override;

  // views::View:
  gfx::Size CalculatePreferredSize() const override;

 private:
  RelaunchRecommendedBubbleView(views::View* anchor_view,
                                base::TimeTicks detection_time);

  // Schedules a timer to fire the next time the title text must be updated; for
  // example, from "...available for 23 hours" to "...available for one day".
  void ScheduleNextTitleRefresh();

  // Invoked when the timer fires to refresh the title text.
  void OnTitleRefresh();

  // The tick count at which Chrome noticed that an update was available. This
  // is used to write the proper string into the dialog's title and to schedule
  // title refreshes to update said string.
  const base::TimeTicks detection_time_;

  // A timer with which title refreshes are scheduled.
  base::OneShotTimer refresh_timer_;

  DISALLOW_COPY_AND_ASSIGN(RelaunchRecommendedBubbleView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_RELAUNCH_NOTIFICATION_RELAUNCH_RECOMMENDED_BUBBLE_VIEW_H_
