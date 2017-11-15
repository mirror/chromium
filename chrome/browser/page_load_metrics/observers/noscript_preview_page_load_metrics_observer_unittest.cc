// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/noscript_preview_page_load_metrics_observer.h"

#include <stdint.h>

#include "base/macros.h"
#include "base/optional.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "chrome/browser/loader/chrome_navigation_data.h"
#include "chrome/browser/page_load_metrics/observers/page_load_metrics_observer_test_harness.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_observer.h"
#include "chrome/browser/page_load_metrics/page_load_tracker.h"
#include "chrome/common/page_load_metrics/page_load_timing.h"
#include "chrome/common/page_load_metrics/test/page_load_metrics_test_util.h"
#include "components/previews/core/previews_features.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/previews_state.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/web_contents_tester.h"

namespace previews {

namespace {

const char kDefaultTestUrl[] = "https://www.google.com";

void SetPreviewsStateOnNavigationHandle(
    content::WebContents* web_contents,
    content::NavigationHandle* navigation_handle,
    content::PreviewsState previews_state) {
  ChromeNavigationData* chrome_navigation_data = new ChromeNavigationData();
  chrome_navigation_data->set_previews_state(previews_state);
  content::WebContentsTester::For(web_contents)
      ->SetNavigationData(navigation_handle,
                          base::WrapUnique(chrome_navigation_data));
}

// Observer subclass to inject navigation data for OnCommit.
class TestNoScriptPreviewPageLoadMetricsObserver
    : public NoScriptPreviewPageLoadMetricsObserver {
 public:
  TestNoScriptPreviewPageLoadMetricsObserver(content::WebContents* web_contents,
                                             bool noscript_used)
      : web_contents_(web_contents), noscript_used_(noscript_used) {}

  ~TestNoScriptPreviewPageLoadMetricsObserver() override {}

  // page_load_metrics::PageLoadMetricsObserver implementation:
  ObservePolicy OnCommit(content::NavigationHandle* navigation_handle,
                         ukm::SourceId source_id) override {
    // Set the navigation state.
    content::PreviewsState previews_state =
        noscript_used_ ? content::NOSCRIPT_ON : content::PREVIEWS_OFF;
    SetPreviewsStateOnNavigationHandle(web_contents_, navigation_handle,
                                       previews_state);

    // Now call the observer method being tested.
    return NoScriptPreviewPageLoadMetricsObserver::OnCommit(navigation_handle,
                                                            source_id);
  }

 private:
  content::WebContents* web_contents_;
  bool noscript_used_;

  DISALLOW_COPY_AND_ASSIGN(TestNoScriptPreviewPageLoadMetricsObserver);
};

class NoScriptPreviewPageLoadMetricsObserverTest
    : public page_load_metrics::PageLoadMetricsObserverTestHarness {
 public:
  NoScriptPreviewPageLoadMetricsObserverTest() : is_using_noscript_(false) {}
  ~NoScriptPreviewPageLoadMetricsObserverTest() override {}

  void ResetTest() {
    page_load_metrics::InitPageLoadTimingForTest(&timing_);
    // Reset to the default testing state. Does not reset histogram state.
    timing_.navigation_start = base::Time::FromDoubleT(1);
    timing_.response_start = base::TimeDelta::FromSeconds(2);
    timing_.parse_timing->parse_start = base::TimeDelta::FromSeconds(3);
    timing_.paint_timing->first_contentful_paint =
        base::TimeDelta::FromSeconds(4);
    timing_.paint_timing->first_paint = base::TimeDelta::FromSeconds(4);
    timing_.paint_timing->first_meaningful_paint =
        base::TimeDelta::FromSeconds(8);
    timing_.paint_timing->first_image_paint = base::TimeDelta::FromSeconds(5);
    timing_.paint_timing->first_text_paint = base::TimeDelta::FromSeconds(6);
    timing_.document_timing->load_event_start = base::TimeDelta::FromSeconds(7);
    timing_.parse_timing->parse_stop = base::TimeDelta::FromSeconds(4);
    timing_.parse_timing->parse_blocked_on_script_load_duration =
        base::TimeDelta::FromSeconds(1);
    PopulateRequiredTimingFields(&timing_);
  }

  void SetIsUsingNoScript(bool is_using_noscript) {
    is_using_noscript_ = is_using_noscript;
  }

  void ValidateTimingHistograms(bool noscript_preview_request_sent) {
    ValidateTimingHistogram(noscript_preview_names::kNavigationToLoadEvent,
                            timing_.document_timing->load_event_start,
                            noscript_preview_request_sent);
    ValidateTimingHistogram(
        noscript_preview_names::kNavigationToFirstContentfulPaint,
        timing_.paint_timing->first_contentful_paint,
        noscript_preview_request_sent);
    ValidateTimingHistogram(
        noscript_preview_names::kNavigationToFirstMeaningfulPaint,
        timing_.paint_timing->first_meaningful_paint,
        noscript_preview_request_sent);
    ValidateTimingHistogram(
        noscript_preview_names::kParseBlockedOnScriptLoad,
        timing_.parse_timing->parse_blocked_on_script_load_duration,
        noscript_preview_request_sent);
    ValidateTimingHistogram(noscript_preview_names::kParseDuration,
                            timing_.parse_timing->parse_stop.value() -
                                timing_.parse_timing->parse_start.value(),
                            noscript_preview_request_sent);
  }

  void ValidateTimingHistogram(const std::string& histogram,
                               const base::Optional<base::TimeDelta>& event,
                               bool noscript_preview_request_sent) {
    histogram_tester().ExpectTotalCount(histogram,
                                        noscript_preview_request_sent ? 1 : 0);
    if (!noscript_preview_request_sent)
      return;
    histogram_tester().ExpectUniqueSample(
        histogram,
        static_cast<base::HistogramBase::Sample>(
            event.value().InMilliseconds()),
        1);
  }

  void ValidateDataHistograms(int network_resources, int64_t network_bytes) {
    if (network_resources > 0) {
      histogram_tester().ExpectUniqueSample(
          noscript_preview_names::kNumNetworkResources, network_resources, 1);
      histogram_tester().ExpectUniqueSample(
          noscript_preview_names::kNetworkBytes,
          static_cast<int>(network_bytes / 1024), 1);
    } else {
      histogram_tester().ExpectTotalCount(
          noscript_preview_names::kNumNetworkResources, 0);
      histogram_tester().ExpectTotalCount(noscript_preview_names::kNetworkBytes,
                                          0);
    }
  }

 protected:
  void RegisterObservers(page_load_metrics::PageLoadTracker* tracker) override {
    tracker->AddObserver(
        base::MakeUnique<TestNoScriptPreviewPageLoadMetricsObserver>(
            web_contents(), is_using_noscript_));
  }

  page_load_metrics::mojom::PageLoadTiming timing_;

 private:
  bool is_using_noscript_;

  DISALLOW_COPY_AND_ASSIGN(NoScriptPreviewPageLoadMetricsObserverTest);
};

TEST_F(NoScriptPreviewPageLoadMetricsObserverTest, NoScriptPreviewNotSeen) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(
      previews::features::kNoScriptPreviews);
  ResetTest();
  SetIsUsingNoScript(false);

  auto navigation_simulator =
      content::NavigationSimulator::CreateRendererInitiated(
          GURL(kDefaultTestUrl), web_contents()->GetMainFrame());
  navigation_simulator->Start();
  navigation_simulator->Commit();

  page_load_metrics::ExtraRequestCompleteInfo main_frame_info(
      GURL(kResourceUrl), net::HostPortPair(), -1 /* frame_tree_node_id */,
      false, /* cached */
      5 * 1024 /* size */, 0 /* original_network_content_length */,
      nullptr
      /* data_reduction_proxy_data */,
      content::RESOURCE_TYPE_MAIN_FRAME, 0, nullptr /* load_timing_info */);
  SimulateLoadedResource(main_frame_info,
                         navigation_simulator->GetGlobalRequestID());

  page_load_metrics::ExtraRequestCompleteInfo cached_resource_info(
      GURL(kResourceUrl), net::HostPortPair(), -1 /* frame_tree_node_id */,
      true, /* cached */
      13 * 1024 /* size */, 0 /* original_network_content_length */,
      nullptr
      /* data_reduction_proxy_data */,
      content::RESOURCE_TYPE_IMAGE, 0, nullptr /* load_timing_info */);
  SimulateLoadedResource(cached_resource_info);

  page_load_metrics::ExtraRequestCompleteInfo network_resource_info(
      GURL(kResourceUrl), net::HostPortPair(), -1 /* frame_tree_node_id */,
      false, /* cached */
      20 * 1024 /* size */, 0 /* original_network_content_length */,
      nullptr
      /* data_reduction_proxy_data */,
      content::RESOURCE_TYPE_IMAGE, 0, nullptr /* load_timing_info */);
  SimulateLoadedResource(network_resource_info);

  SimulateTimingUpdate(timing_);
  NavigateToUntrackedUrl();

  ValidateTimingHistograms(false /* is_using_noscript */);
  ValidateDataHistograms(0 /* network_resources */, 0 /* network_bytes */);
}

TEST_F(NoScriptPreviewPageLoadMetricsObserverTest, NoScriptPreviewSeen) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(
      previews::features::kNoScriptPreviews);
  ResetTest();
  SetIsUsingNoScript(true);

  auto navigation_simulator =
      content::NavigationSimulator::CreateRendererInitiated(
          GURL(kDefaultTestUrl), web_contents()->GetMainFrame());
  navigation_simulator->Start();
  navigation_simulator->Commit();

  page_load_metrics::ExtraRequestCompleteInfo main_frame_info(
      GURL(kResourceUrl), net::HostPortPair(), -1 /* frame_tree_node_id */,
      false, /* cached */
      5 * 1024 /* size */, 0 /* original_network_content_length */,
      nullptr
      /* data_reduction_proxy_data */,
      content::RESOURCE_TYPE_MAIN_FRAME, 0, nullptr /* load_timing_info */);
  SimulateLoadedResource(main_frame_info,
                         navigation_simulator->GetGlobalRequestID());

  page_load_metrics::ExtraRequestCompleteInfo cached_resource_info(
      GURL(kResourceUrl), net::HostPortPair(), -1 /* frame_tree_node_id */,
      true, /* cached */
      13 * 1024 /* size */, 0 /* original_network_content_length */,
      nullptr
      /* data_reduction_proxy_data */,
      content::RESOURCE_TYPE_IMAGE, 0, nullptr /* load_timing_info */);
  SimulateLoadedResource(cached_resource_info);

  page_load_metrics::ExtraRequestCompleteInfo network_resource_info(
      GURL(kResourceUrl), net::HostPortPair(), -1 /* frame_tree_node_id */,
      false, /* cached */
      20 * 1024 /* size */, 0 /* original_network_content_length */,
      nullptr
      /* data_reduction_proxy_data */,
      content::RESOURCE_TYPE_IMAGE, 0, nullptr /* load_timing_info */);
  SimulateLoadedResource(network_resource_info);

  SimulateTimingUpdate(timing_);
  NavigateToUntrackedUrl();

  ValidateTimingHistograms(true /* is_using_noscript */);
  ValidateDataHistograms(2 /* network_resources */,
                         25 * 1024 /* network_bytes */);
}

}  // namespace

}  //  namespace previews
