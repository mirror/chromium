// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/run_loop.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/resource_coordinator/resource_coordinator_render_process_probe.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/test/test_utils.h"
#include "url/gurl.h"

namespace resource_coordinator {

class ResourceCoordinatorRenderProcessProbeBrowserTest
    : public InProcessBrowserTest {
 public:
  content::WebContents* LoadTabAndWait(
      const std::string& url,
      WindowOpenDisposition window_disposition) {
    content::WindowedNotificationObserver load(
        content::NOTIFICATION_NAV_ENTRY_COMMITTED,
        content::NotificationService::AllSources());
    content::OpenURLParams open(GURL(url), content::Referrer(),
                                window_disposition, ui::PAGE_TRANSITION_TYPED,
                                false);
    content::WebContents* tab = browser()->OpenURL(open);
    load.Wait();
    return tab;
  }

  void KillTabAndWait(content::WebContents* web_contents) {
    content::WindowedNotificationObserver close(
        content::NOTIFICATION_RENDERER_PROCESS_CLOSED,
        content::NotificationService::AllSources());
    web_contents->GetRenderProcessHost()->FastShutdownForPageCount(1u);
    close.Wait();
  }
};

class MockResourceCoordinatorRenderProcessProbe
    : public ResourceCoordinatorRenderProcessProbe {
 public:
  MockResourceCoordinatorRenderProcessProbe() = default;
  ~MockResourceCoordinatorRenderProcessProbe() override = default;

  // Terminate the message loop at the end of the gather cycle.
  void HandleRenderProcessMetrics() override {
    base::MessageLoop::current()->QuitWhenIdle();
  }

  // Check to see if the measurements for all of the currently
  // tracked render processes are up-to-date.
  bool AllRenderProcessMeasurementsAreCurrent() {
    for (auto& render_process_info_map_entry : render_process_info_map_) {
      auto& render_process_info = render_process_info_map_entry.second;
      if (render_process_info.last_gather_cycle_active !=
              current_gather_cycle_ ||
          render_process_info.cpu_usage < 0.0) {
        return false;
      }
    }
    return true;
  }

  // Accessors for testing.
  size_t current_gather_cycle() { return current_gather_cycle_; }
  const RenderProcessInfoMap& render_process_info_map() {
    return render_process_info_map_;
  }
};

IN_PROC_BROWSER_TEST_F(ResourceCoordinatorRenderProcessProbeBrowserTest,
                       TrackAndMeasureActiveRenderProcesses) {
  MockResourceCoordinatorRenderProcessProbe probe;
  EXPECT_EQ(0u, probe.current_gather_cycle());

  base::RunLoop loop1;
  auto* tab1 = LoadTabAndWait(chrome::kChromeUIAboutURL,
                              WindowOpenDisposition::CURRENT_TAB);
  probe.StartGatherCycle();
  loop1.Run();
  EXPECT_EQ(1u, probe.current_gather_cycle());
  EXPECT_EQ(1u, probe.render_process_info_map().size());
  EXPECT_TRUE(probe.AllRenderProcessMeasurementsAreCurrent());

  base::RunLoop loop2;
  auto* tab2 = LoadTabAndWait(chrome::kChromeUICreditsURL,
                              WindowOpenDisposition::NEW_FOREGROUND_TAB);
  probe.StartGatherCycle();
  loop2.Run();
  EXPECT_EQ(2u, probe.current_gather_cycle());
  EXPECT_EQ(2u, probe.render_process_info_map().size());
  EXPECT_TRUE(probe.AllRenderProcessMeasurementsAreCurrent());

  base::RunLoop loop3;
  KillTabAndWait(tab1);
  probe.StartGatherCycle();
  loop3.Run();
  EXPECT_EQ(3u, probe.current_gather_cycle());
  EXPECT_EQ(1u, probe.render_process_info_map().size());
  EXPECT_TRUE(probe.AllRenderProcessMeasurementsAreCurrent());

  base::RunLoop loop4;
  KillTabAndWait(tab2);
  probe.StartGatherCycle();
  loop4.Run();
  EXPECT_EQ(4u, probe.current_gather_cycle());
  EXPECT_EQ(0u, probe.render_process_info_map().size());
  EXPECT_TRUE(probe.AllRenderProcessMeasurementsAreCurrent());
}

}  // namespace resource_coordinator
