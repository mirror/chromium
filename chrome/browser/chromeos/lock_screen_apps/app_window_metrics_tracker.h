// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOCK_SCREEN_APPS_APP_WINDOW_METRICS_TRACKER_H_
#define CHROME_BROWSER_CHROMEOS_LOCK_SCREEN_APPS_APP_WINDOW_METRICS_TRACKER_H_

#include <map>

#include "base/macros.h"
#include "base/time/time.h"
#include "content/public/browser/web_contents_observer.h"

namespace base {
class TickClock;
}

namespace extensions {
class AppWindow;
}

namespace lock_screen_apps {

class AppWindowMetricsTracker : public content::WebContentsObserver {
 public:
  explicit AppWindowMetricsTracker(base::TickClock* clock);
  ~AppWindowMetricsTracker() override;

  void AppLaunchRequested();
  void MovedToForeground();
  void MovedToBackground();
  void AppWindowCreated(extensions::AppWindow* app_window);
  void Reset();

  // content::WebContentsObserver:
  void RenderViewCreated(content::RenderViewHost* render_view_host) override;
  void DocumentLoadedInFrame(
      content::RenderFrameHost* render_frame_host) override;
  void WebContentsDestroyed() override;

 private:
  // NOTE: Used in histograms - do not change order, or remove entries.
  // Also, update LockScreenAppSessionState enum.
  enum class State {
    kInitial = 0,
    kLaunchRequested = 1,
    kWindowCreated = 2,
    kWindowShown = 3,
    kForeground = 4,
    kBackground = 5,
    kWindowClosed = 6,

    kCount
  };

  void SetState(State state);

  base::TickClock* clock_;

  State state_ = State::kInitial;

  std::map<State, base::TimeTicks> time_stamps_;

  extensions::AppWindow* app_window_ = nullptr;

  // Number of times app launch was requested during the
  int app_launch_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(AppWindowMetricsTracker);
};

}  // namespace lock_screen_apps

#endif  // CHROME_BROWSER_CHROMEOS_LOCK_SCREEN_APPS_APP_WINDOW_METRICS_TRACKER_H_
