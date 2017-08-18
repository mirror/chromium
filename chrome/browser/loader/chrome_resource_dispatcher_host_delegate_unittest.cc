// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/loader/chrome_resource_dispatcher_host_delegate.h"

#include <memory>

#include "base/run_loop.h"
#include "base/test/simple_test_clock.h"
#include "base/test/simple_test_tick_clock.h"
#include "base/time/time.h"
#include "chrome/browser/loader/chrome_navigation_data.h"
#include "chrome/browser/loader/chrome_resource_dispatcher_host_delegate.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_data.h"
#include "components/network_time/network_time_test_utils.h"
#include "components/network_time/network_time_tracker.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_data.h"
#include "content/public/test/test_browser_thread.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/base/request_priority.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

class ChromeResourceDispatcherHostDelegateTest : public testing::Test {
 public:
  ChromeResourceDispatcherHostDelegateTest()
      : profile_manager_(
            new TestingProfileManager(TestingBrowserProcess::GetGlobal())) {}
  ~ChromeResourceDispatcherHostDelegateTest() override {}

  void SetUp() override { ASSERT_TRUE(profile_manager_->SetUp()); }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  // Set up TestingProfileManager for extensions::UserScriptListener.
  std::unique_ptr<TestingProfileManager> profile_manager_;
};

TEST_F(ChromeResourceDispatcherHostDelegateTest,
       GetNavigationDataWithDataReductionProxyData) {
  std::unique_ptr<net::URLRequestContext> context(new net::URLRequestContext());
  std::unique_ptr<net::URLRequest> fake_request(
      context->CreateRequest(GURL("google.com"), net::RequestPriority::IDLE,
                             nullptr, TRAFFIC_ANNOTATION_FOR_TESTS));
  // Add DataReductionProxyData to URLRequest
  data_reduction_proxy::DataReductionProxyData* data_reduction_proxy_data =
      data_reduction_proxy::DataReductionProxyData::GetDataAndCreateIfNecessary(
          fake_request.get());
  data_reduction_proxy_data->set_used_data_reduction_proxy(true);
  std::unique_ptr<ChromeResourceDispatcherHostDelegate> delegate(
      new ChromeResourceDispatcherHostDelegate());
  ChromeNavigationData* chrome_navigation_data =
      static_cast<ChromeNavigationData*>(
          delegate->GetNavigationData(fake_request.get()));
  data_reduction_proxy::DataReductionProxyData* data_reduction_proxy_data_copy =
      chrome_navigation_data->GetDataReductionProxyData();
  // The DataReductionProxyData should be a copy of the one on URLRequest
  EXPECT_NE(data_reduction_proxy_data_copy, data_reduction_proxy_data);
  // Make sure DataReductionProxyData was copied.
  EXPECT_TRUE(data_reduction_proxy_data_copy->used_data_reduction_proxy());
  EXPECT_EQ(
      chrome_navigation_data,
      ChromeNavigationData::GetDataAndCreateIfNecessary(fake_request.get()));
}

TEST_F(ChromeResourceDispatcherHostDelegateTest,
       GetNavigationDataWithoutDataReductionProxyData) {
  std::unique_ptr<net::URLRequestContext> context(new net::URLRequestContext());
  std::unique_ptr<net::URLRequest> fake_request(
      context->CreateRequest(GURL("google.com"), net::RequestPriority::IDLE,
                             nullptr, TRAFFIC_ANNOTATION_FOR_TESTS));
  std::unique_ptr<ChromeResourceDispatcherHostDelegate> delegate(
      new ChromeResourceDispatcherHostDelegate());
  ChromeNavigationData* chrome_navigation_data =
      static_cast<ChromeNavigationData*>(
          delegate->GetNavigationData(fake_request.get()));
  EXPECT_FALSE(chrome_navigation_data->GetDataReductionProxyData());
}

class ChromeResourceDispatcherHostDelegateTimeTrackerTest
    : public ChromeRenderViewHostTestHarness {
 public:
  ChromeResourceDispatcherHostDelegateTimeTrackerTest()
      : clock_(new base::SimpleTestClock),
        tick_clock_(new base::SimpleTestTickClock),
        test_server_(new net::EmbeddedTestServer),
        field_trial_test_(new network_time::FieldTrialTest()) {
    SetThreadBundleOptions(content::TestBrowserThreadBundle::REAL_IO_THREAD);
    network_time::NetworkTimeTracker::RegisterPrefs(pref_service_.registry());
  }
  ~ChromeResourceDispatcherHostDelegateTimeTrackerTest() override {}

  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    tracker_.reset(new network_time::NetworkTimeTracker(
        std::unique_ptr<base::Clock>(clock_),
        std::unique_ptr<base::TickClock>(tick_clock_), &pref_service_,
        new net::TestURLRequestContextGetter(
            content::BrowserThread::GetTaskRunnerForThread(
                content::BrowserThread::IO))));
    // Do this to be sure that |is_null| returns false.
    clock_->Advance(base::TimeDelta::FromDays(111));
    tick_clock_->Advance(base::TimeDelta::FromDays(222));
  }

  void TearDown() override { ChromeRenderViewHostTestHarness::TearDown(); }

  void WaitForUIThread() {
    base::RunLoop run_loop;
    content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
                                     run_loop.QuitClosure());
    run_loop.Run();
  }

  network_time::NetworkTimeTracker* tracker() { return tracker_.get(); }
  net::EmbeddedTestServer* test_server() { return test_server_.get(); }
  network_time::FieldTrialTest* field_trial_test() {
    return field_trial_test_.get();
  }

 private:
  base::SimpleTestClock* clock_;
  base::SimpleTestTickClock* tick_clock_;

  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<network_time::NetworkTimeTracker> tracker_;
  std::unique_ptr<net::EmbeddedTestServer> test_server_;
  std::unique_ptr<network_time::FieldTrialTest> field_trial_test_;
};

TEST_F(ChromeResourceDispatcherHostDelegateTimeTrackerTest, TimeQueryStarted) {
  std::unique_ptr<ChromeResourceDispatcherHostDelegate> delegate(
      new ChromeResourceDispatcherHostDelegate());

  base::Time network_time;
  base::TimeDelta uncertainty;
  EXPECT_EQ(network_time::NetworkTimeTracker::NETWORK_TIME_NO_SYNC_ATTEMPT,
            tracker()->GetNetworkTime(&network_time, &uncertainty));

  // Enable network time queries.
  test_server()->RegisterRequestHandler(
      base::Bind(&network_time::GoodTimeResponseHandler));
  EXPECT_TRUE(test_server()->Start());
  tracker()->SetTimeServerURLForTesting(test_server()->GetURL("/"));
  field_trial_test()->SetNetworkQueriesWithVariationsService(
      true, 0.0, network_time::NetworkTimeTracker::FETCHES_ON_DEMAND_ONLY);
  delegate->StartNetworkTimeTrackerFetch(tracker());
  WaitForUIThread();

  tracker()->WaitForFetchForTesting(123123123);
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(network_time::NetworkTimeTracker::NETWORK_TIME_AVAILABLE,
            tracker()->GetNetworkTime(&network_time, &uncertainty));
}
