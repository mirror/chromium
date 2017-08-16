// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/feature_list.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/resource_coordinator/resource_coordinator_process_probe.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/test/test_utils.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_features.h"
#include "url/gurl.h"

namespace resource_coordinator {

class MockResourceCoordinatorProcessMetricsHandler
    : public ProcessMetricsHandler {
 public:
  MockResourceCoordinatorProcessMetricsHandler() = default;
  ~MockResourceCoordinatorProcessMetricsHandler() override = default;

  bool HandleMetrics(
      const BrowserProcessInfo& browser_process_info,
      const RenderProcessInfoMap& render_process_info_map) override {
    // TODO(ducbui): fix the presubmit error on the following banned function.
    // base::RunLoop:: Quit CurrentWhenIdleDeprecated();
    return false;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MockResourceCoordinatorProcessMetricsHandler);
};

class ResourceCoordinatorProcessProbeBrowserTest : public InProcessBrowserTest {
 public:
  ResourceCoordinatorProcessProbeBrowserTest() = default;
  ~ResourceCoordinatorProcessProbeBrowserTest() override = default;

  void StartGatherCycleAndWait() {
    base::RunLoop run_loop;
    probe_->StartGatherCycle();
    run_loop.Run();
  }

  void set_probe(resource_coordinator::ResourceCoordinatorProcessProbe* probe) {
    probe_ = probe;
  }

 private:
  resource_coordinator::ResourceCoordinatorProcessProbe* probe_;

  DISALLOW_COPY_AND_ASSIGN(ResourceCoordinatorProcessProbeBrowserTest);
};

IN_PROC_BROWSER_TEST_F(ResourceCoordinatorProcessProbeBrowserTest,
                       TrackAndMeasureActiveRenderProcesses) {
  // Ensure that the |resource_coordinator| service is enabled.
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeatures({features::kGlobalResourceCoordinator,
                                 features::kGRCRenderProcessCPUProfiling},
                                {});

  resource_coordinator::ResourceCoordinatorProcessProbe probe;
  probe.set_render_process_metrics_handler_for_testing(
      base::MakeUnique<MockResourceCoordinatorProcessMetricsHandler>());
  set_probe(&probe);

  ASSERT_TRUE(embedded_test_server()->Start());
  EXPECT_EQ(0u, probe.current_gather_cycle_for_testing());

  // A tab is already open when the test begins.
  StartGatherCycleAndWait();
  EXPECT_EQ(1u, probe.current_gather_cycle_for_testing());
  EXPECT_EQ(1u, probe.render_process_info_map_for_testing().size());
  EXPECT_TRUE(probe.AllRenderProcessMeasurementsAreCurrentForTesting());

  // Open a second tab and complete a navigation.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), embedded_test_server()->GetURL("/title1.html"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_TAB |
          ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  StartGatherCycleAndWait();
  EXPECT_EQ(2u, probe.current_gather_cycle_for_testing());
  EXPECT_EQ(2u, probe.render_process_info_map_for_testing().size());
  EXPECT_TRUE(probe.AllRenderProcessMeasurementsAreCurrentForTesting());

  // Kill one of the two open tabs.
  EXPECT_TRUE(browser()
                  ->tab_strip_model()
                  ->GetActiveWebContents()
                  ->GetRenderProcessHost()
                  ->FastShutdownIfPossible());
  StartGatherCycleAndWait();
  EXPECT_EQ(3u, probe.current_gather_cycle_for_testing());
  EXPECT_EQ(1u, probe.render_process_info_map_for_testing().size());
  EXPECT_TRUE(probe.AllRenderProcessMeasurementsAreCurrentForTesting());
}

}  // namespace resource_coordinator
