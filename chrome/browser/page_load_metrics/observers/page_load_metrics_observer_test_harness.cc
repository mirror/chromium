// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/page_load_metrics_observer_test_harness.h"

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/metrics_hashes.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_embedder_interface.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_util.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_data.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/test/web_contents_tester.h"
#include "third_party/WebKit/public/platform/WebInputEvent.h"

namespace page_load_metrics {

namespace {

class TestPageLoadMetricsEmbedderInterface
    : public PageLoadMetricsEmbedderInterface {
 public:
  explicit TestPageLoadMetricsEmbedderInterface(
      PageLoadMetricsObserverTestHarness* test)
      : test_(test) {}

  bool IsNewTabPageUrl(const GURL& url) override { return false; }

  // Forward the registration logic to the test class so that derived classes
  // can override the logic there without depending on the embedder interface.
  void RegisterObservers(PageLoadTracker* tracker) override {
    test_->RegisterObservers(tracker);
  }

  std::unique_ptr<base::Timer> CreateTimer() override {
    auto timer = base::MakeUnique<test::WeakMockTimer>();
    test_->SetMockTimer(timer->AsWeakPtr());
    return std::move(timer);
  }

 private:
  PageLoadMetricsObserverTestHarness* test_;

  DISALLOW_COPY_AND_ASSIGN(TestPageLoadMetricsEmbedderInterface);
};

}  // namespace

PageLoadMetricsObserverTestHarness::PageLoadMetricsObserverTestHarness()
    : ChromeRenderViewHostTestHarness() {}

PageLoadMetricsObserverTestHarness::~PageLoadMetricsObserverTestHarness() {}

void PageLoadMetricsObserverTestHarness::SetUp() {
  ChromeRenderViewHostTestHarness::SetUp();
  ukm_tester_.SetUp();
  SetContents(CreateTestWebContents());
  NavigateAndCommit(GURL("http://www.google.com"));
  observer_ = MetricsWebContentsObserver::CreateForWebContents(
      web_contents(), base::nullopt,
      base::MakeUnique<TestPageLoadMetricsEmbedderInterface>(this));
  web_contents()->WasShown();
}

void PageLoadMetricsObserverTestHarness::StartNavigation(const GURL& gurl) {
  content::WebContentsTester* web_contents_tester =
      content::WebContentsTester::For(web_contents());
  web_contents_tester->StartNavigation(gurl);
}

void PageLoadMetricsObserverTestHarness::SimulateTimingUpdate(
    const mojom::PageLoadTiming& timing) {
  SimulateTimingAndMetadataUpdate(timing, mojom::PageLoadMetadata());
}

void PageLoadMetricsObserverTestHarness::SimulateTimingAndMetadataUpdate(
    const mojom::PageLoadTiming& timing,
    const mojom::PageLoadMetadata& metadata) {
  observer_->OnTimingUpdated(web_contents()->GetMainFrame(), timing, metadata);
  // If sending the timing update caused the PageLoadMetricsUpdateDispatcher to
  // schedule a buffering timer, then fire it now so metrics are dispatched to
  // observers.
  base::MockTimer* mock_timer = GetMockTimer();
  if (mock_timer && mock_timer->IsRunning())
    mock_timer->Fire();
}

void PageLoadMetricsObserverTestHarness::SimulateLoadedResource(
    const ExtraRequestCompleteInfo& info,
    const content::GlobalRequestID& request_id) {
  if (info.resource_type == content::RESOURCE_TYPE_MAIN_FRAME) {
    ASSERT_NE(content::GlobalRequestID(), request_id)
        << "Main frame resources must have a GlobalRequestID.";
  }

  observer_->OnRequestComplete(
      info.url, info.host_port_pair, info.frame_tree_node_id, request_id,
      info.resource_type, info.was_cached,
      info.data_reduction_proxy_data
          ? info.data_reduction_proxy_data->DeepCopy()
          : nullptr,
      info.raw_body_bytes, info.original_network_content_length,
      base::TimeTicks::Now(), 0);
}

void PageLoadMetricsObserverTestHarness::SimulateInputEvent(
    const blink::WebInputEvent& event) {
  observer_->OnInputEvent(event);
}

void PageLoadMetricsObserverTestHarness::SimulateAppEnterBackground() {
  observer_->FlushMetricsOnAppEnterBackground();
}

void PageLoadMetricsObserverTestHarness::SimulateMediaPlayed() {
  content::WebContentsObserver::MediaPlayerInfo video_type(
      true /* has_video*/, true /* has_audio */);
  content::RenderFrameHost* render_frame_host = web_contents()->GetMainFrame();
  observer_->MediaStartedPlaying(video_type,
                                 std::make_pair(render_frame_host, 0));
}

const base::HistogramTester&
PageLoadMetricsObserverTestHarness::histogram_tester() const {
  return histogram_tester_;
}

MetricsWebContentsObserver* PageLoadMetricsObserverTestHarness::observer()
    const {
  return observer_;
}

const PageLoadExtraInfo
PageLoadMetricsObserverTestHarness::GetPageLoadExtraInfoForCommittedLoad() {
  return observer_->GetPageLoadExtraInfoForCommittedLoad();
}

void PageLoadMetricsObserverTestHarness::NavigateWithPageTransitionAndCommit(
    const GURL& url,
    ui::PageTransition transition) {
  controller().LoadURL(url, content::Referrer(), transition, std::string());
  content::WebContentsTester::For(web_contents())->CommitPendingNavigation();
}

void PageLoadMetricsObserverTestHarness::UkmTester::SetUp() {
  TestingBrowserProcess::GetGlobal()->SetUkmRecorder(&test_ukm_recorder_);
}

const ukm::UkmSource*
PageLoadMetricsObserverTestHarness::UkmTester::GetSourceForUrl(
    const char* url) const {
  std::vector<const ukm::UkmSource*> matching_sources = GetSourcesForUrl(url);
  EXPECT_LE(1ul, matching_sources.size());
  return matching_sources.empty() ? nullptr : matching_sources.back();
}

std::vector<const ukm::UkmSource*>
PageLoadMetricsObserverTestHarness::UkmTester::GetSourcesForUrl(
    const char* url) const {
  std::vector<const ukm::UkmSource*> matching_sources;
  for (const auto& candidate : test_ukm_recorder_.GetSources()) {
    if (candidate.second->url() == url)
      matching_sources.push_back(candidate.second.get());
  }
  return matching_sources;
}

std::vector<const ukm::mojom::UkmEntry*>
PageLoadMetricsObserverTestHarness::UkmTester::GetEntriesForSourceID(
    ukm::SourceId source_id,
    const char* event) const {
  std::vector<const ukm::mojom::UkmEntry*> entries;
  for (size_t i = 0; i < entries_count(); ++i) {
    const ukm::mojom::UkmEntry* entry = test_ukm_recorder_.GetEntry(i);
    if (entry->source_id == source_id &&
        entry->event_hash == base::HashMetricName(event)) {
      entries.push_back(entry);
    }
  }
  return entries;
}

// static
ukm::mojom::UkmEntryPtr
PageLoadMetricsObserverTestHarness::UkmTester::GetMergedEntry(
    const std::vector<const ukm::mojom::UkmEntry*>& entries) {
  EXPECT_FALSE(entries.empty());
  ukm::mojom::UkmEntryPtr merged_entry = ukm::mojom::UkmEntry::New();
  for (const auto* entry : entries) {
    if (merged_entry->event_hash) {
      EXPECT_EQ(merged_entry->source_id, entry->source_id);
      EXPECT_EQ(merged_entry->event_hash, entry->event_hash);
    } else {
      merged_entry->event_hash = entry->event_hash;
      merged_entry->source_id = entry->source_id;
    }
    for (const auto& metric : entry->metrics) {
      merged_entry->metrics.emplace_back(metric->Clone());
    }
  }
  return merged_entry;
}

ukm::mojom::UkmEntryPtr
PageLoadMetricsObserverTestHarness::UkmTester::GetMergedEntryForSourceID(
    ukm::SourceId source_id,
    const char* event) const {
  ukm::mojom::UkmEntryPtr entry =
      GetMergedEntry(GetEntriesForSourceID(source_id, event));
  EXPECT_EQ(source_id, entry->source_id);
  EXPECT_EQ(base::HashMetricName(event), entry->event_hash);
  return entry;
}

bool PageLoadMetricsObserverTestHarness::UkmTester::HasEntry(
    const ukm::UkmSource& source,
    const char* event) const {
  return !GetEntriesForSourceID(source.id(), event).empty();
}

int PageLoadMetricsObserverTestHarness::UkmTester::CountMetrics(
    const ukm::UkmSource& source,
    const char* event) const {
  ukm::mojom::UkmEntryPtr entry = GetMergedEntryForSourceID(source.id(), event);
  return entry.get() ? entry->metrics.size() : 0;
}

bool PageLoadMetricsObserverTestHarness::UkmTester::HasMetric(
    const ukm::UkmSource& source,
    const char* event,
    const char* name) const {
  ukm::mojom::UkmEntryPtr entry = GetMergedEntryForSourceID(source.id(), event);
  return ukm::TestUkmRecorder::FindMetric(entry.get(), name) != nullptr;
}

void PageLoadMetricsObserverTestHarness::UkmTester::ExpectMetric(
    const ukm::UkmSource& source,
    const char* event,
    const char* name,
    int64_t expected_value) const {
  ukm::mojom::UkmEntryPtr entry = GetMergedEntryForSourceID(source.id(), event);
  const ukm::mojom::UkmMetric* metric =
      ukm::TestUkmRecorder::FindMetric(entry.get(), name);
  EXPECT_NE(nullptr, metric) << "Failed to find metric: " << name;
  EXPECT_EQ(expected_value, metric->value);
}

const char PageLoadMetricsObserverTestHarness::kResourceUrl[] =
    "https://www.example.com/resource";

}  // namespace page_load_metrics
