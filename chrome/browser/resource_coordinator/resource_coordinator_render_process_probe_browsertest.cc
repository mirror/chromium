// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/resource_coordinator/resource_coordinator_render_process_probe.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/test/test_utils.h"
#include "url/gurl.h"

namespace resource_coordinator {

class MockResourceCoordinatorRenderProcessMetricsHandler
    : public RenderProcessMetricsHandler {
 public:
  MockResourceCoordinatorRenderProcessMetricsHandler() = default;
  ~MockResourceCoordinatorRenderProcessMetricsHandler() override = default;

  bool HandleMetrics(
      const RenderProcessInfoMap& render_process_info_map) override {
    base::MessageLoop::current()->QuitWhenIdle();
    return false;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MockResourceCoordinatorRenderProcessMetricsHandler);
};

class MockResourceCoordinatorRenderProcessProbe
    : public ResourceCoordinatorRenderProcessProbe {
 public:
  MockResourceCoordinatorRenderProcessProbe()
      : ResourceCoordinatorRenderProcessProbe(
            base::MakeUnique<
                MockResourceCoordinatorRenderProcessMetricsHandler>()) {}
  ~MockResourceCoordinatorRenderProcessProbe() override = default;

  // Check to see if the measurements for all of the currently
  // tracked render processes are up-to-date.
  bool AllRenderProcessMeasurementsAreCurrent() const {
    for (auto& render_process_info_map_entry : render_process_info_map()) {
      auto& render_process_info = render_process_info_map_entry.second;
      if (render_process_info.last_gather_cycle_active !=
              current_gather_cycle() ||
          render_process_info.cpu_usage < 0.0) {
        return false;
      }
    }
    return true;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MockResourceCoordinatorRenderProcessProbe);
};

class ResourceCoordinatorRenderProcessProbeBrowserTest
    : public InProcessBrowserTest {
 public:
  ResourceCoordinatorRenderProcessProbeBrowserTest() = default;
  ~ResourceCoordinatorRenderProcessProbeBrowserTest() override = default;

  void StartGatherCycleAndWait() {
    base::RunLoop run_loop;
    probe_.StartGatherCycle();
    run_loop.Run();
  }

  const MockResourceCoordinatorRenderProcessProbe& probe() { return probe_; }

 private:
  MockResourceCoordinatorRenderProcessProbe probe_;

  DISALLOW_COPY_AND_ASSIGN(ResourceCoordinatorRenderProcessProbeBrowserTest);
};

IN_PROC_BROWSER_TEST_F(ResourceCoordinatorRenderProcessProbeBrowserTest,
                       TrackAndMeasureActiveRenderProcesses) {
  EXPECT_EQ(0u, probe().current_gather_cycle());

  // A tab is already open when the test begins.
  StartGatherCycleAndWait();
  EXPECT_EQ(1u, probe().current_gather_cycle());
  EXPECT_EQ(1u, probe().render_process_info_map().size());
  EXPECT_TRUE(probe().AllRenderProcessMeasurementsAreCurrent());

  // Open a second tab and complete a navigation.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(chrome::kChromeUICreditsURL),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_TAB |
          ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  StartGatherCycleAndWait();
  EXPECT_EQ(2u, probe().current_gather_cycle());
  EXPECT_EQ(2u, probe().render_process_info_map().size());
  EXPECT_TRUE(probe().AllRenderProcessMeasurementsAreCurrent());

  // Kill one of the two open tabs.
  EXPECT_TRUE(browser()
                  ->tab_strip_model()
                  ->GetActiveWebContents()
                  ->GetRenderProcessHost()
                  ->FastShutdownIfPossible());
  StartGatherCycleAndWait();
  EXPECT_EQ(3u, probe().current_gather_cycle());
  EXPECT_EQ(1u, probe().render_process_info_map().size());
  EXPECT_TRUE(probe().AllRenderProcessMeasurementsAreCurrent());
}

}  // namespace resource_coordinator
