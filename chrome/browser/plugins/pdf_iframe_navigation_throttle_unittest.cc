// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/plugins/pdf_iframe_navigation_throttle.h"

#include "base/test/scoped_feature_list.h"
#include "chrome/browser/plugins/chrome_plugin_service_filter.h"
#include "chrome/common/chrome_features.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/plugin_service.h"
#include "net/http/http_util.h"

namespace {
const char kHeader[] = "HTTP/1.1 200 OK\r\n";
const char kPDFHeader[] =
    "HTTP/1.1 200 OK\r\n"
    "content-type: application/pdf\r\n";
const char kExampleURL[] = "http://example.com";
}  // namespace

void PluginsLoadedCallback(const base::Closure& callback,
                           const std::vector<content::WebPluginInfo>& plugins) {
  callback.Run();
}

class PDFIFrameNavigationThrottleTest : public ChromeRenderViewHostTestHarness {
 protected:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();

    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndEnableFeature(features::kClickToOpenPDFPlaceholder);

    content::PluginService::GetInstance()->Init();

    // Load plugins.
    base::RunLoop run_loop;
    content::PluginService::GetInstance()->GetPlugins(
        base::Bind(&PluginsLoadedCallback, run_loop.QuitClosure()));
    run_loop.Run();

    content::RenderFrameHostTester::For(main_rfh())
        ->InitializeRenderFrameIfNeeded();
    subframe_ = content::RenderFrameHostTester::For(main_rfh())
                    ->AppendChild("subframe");
  }

  void SetAlwaysOpenPdfExternallyForTests(bool always_open_pdf_externally) {
    PluginPrefs::GetForTestingProfile(profile())
        ->SetAlwaysOpenPdfExternallyForTests(always_open_pdf_externally);
  }

  content::RenderFrameHost* subframe_;
};

TEST_F(PDFIFrameNavigationThrottleTest, CreateThrottle) {
  std::unique_ptr<content::NavigationHandle> handle =
      content::NavigationHandle::CreateNavigationHandleForTesting(
          GURL(kExampleURL), main_rfh());

  handle->CallWillProcessResponseForTesting(
      main_rfh(), net::HttpUtil::AssembleRawHeaders(kHeader, strlen(kHeader)));

  std::unique_ptr<content::NavigationThrottle> throttle =
      PDFIFrameNavigationThrottle::MaybeCreateThrottleFor(handle.get());
  ASSERT_EQ(nullptr, throttle);

  handle = content::NavigationHandle::CreateNavigationHandleForTesting(
      GURL(kExampleURL), subframe_);

  handle->CallWillProcessResponseForTesting(
      subframe_, net::HttpUtil::AssembleRawHeaders(kHeader, strlen(kHeader)));

  throttle = PDFIFrameNavigationThrottle::MaybeCreateThrottleFor(handle.get());
  ASSERT_NE(nullptr, throttle);
}

TEST_F(PDFIFrameNavigationThrottleTest, InterceptPDFOnly) {
  std::unique_ptr<content::NavigationHandle> handle =
      content::NavigationHandle::CreateNavigationHandleForTesting(
          GURL(kExampleURL), subframe_);

  // Using header with blank mime type.
  handle->CallWillProcessResponseForTesting(
      subframe_, net::HttpUtil::AssembleRawHeaders(kHeader, strlen(kHeader)));

  std::unique_ptr<content::NavigationThrottle> throttle =
      PDFIFrameNavigationThrottle::MaybeCreateThrottleFor(handle.get());

  EXPECT_NE(nullptr, throttle);
  ASSERT_EQ(content::NavigationThrottle::PROCEED,
            throttle->WillProcessResponse());
}

TEST_F(PDFIFrameNavigationThrottleTest, CancelOnlyIfPDFViewerIsDisabled) {
  std::unique_ptr<content::NavigationHandle> handle =
      content::NavigationHandle::CreateNavigationHandleForTesting(
          GURL(kExampleURL), subframe_);

  handle->CallWillProcessResponseForTesting(
      subframe_,
      net::HttpUtil::AssembleRawHeaders(kPDFHeader, strlen(kPDFHeader)));

  std::unique_ptr<content::NavigationThrottle> throttle =
      PDFIFrameNavigationThrottle::MaybeCreateThrottleFor(handle.get());

  SetAlwaysOpenPdfExternallyForTests(false);
  ChromePluginServiceFilter* filter = ChromePluginServiceFilter::GetInstance();
  filter->RegisterResourceContext(profile(), profile()->GetResourceContext());

  EXPECT_NE(nullptr, throttle);
  ASSERT_EQ(content::NavigationThrottle::PROCEED,
            throttle->WillProcessResponse());

  SetAlwaysOpenPdfExternallyForTests(true);
  filter->RegisterResourceContext(profile(), profile()->GetResourceContext());

  EXPECT_NE(nullptr, throttle);
  ASSERT_EQ(content::NavigationThrottle::CANCEL_AND_IGNORE,
            throttle->WillProcessResponse());
}
