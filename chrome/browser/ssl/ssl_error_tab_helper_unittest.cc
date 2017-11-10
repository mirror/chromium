// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/ssl_error_tab_helper.h"

#include "base/bind.h"
#include "base/time/time.h"
#include "chrome/browser/ssl/ssl_blocking_page.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/browser/certificate_request_result_type.h"
#include "content/public/browser/navigation_handle.h"
#include "net/base/net_errors.h"
#include "net/test/cert_test_util.h"
#include "net/test/test_data_directory.h"
#include "url/gurl.h"

namespace {

class SSLErrorTabHelperTest : public ChromeRenderViewHostTestHarness {
 protected:
  std::unique_ptr<content::NavigationHandle> CreateHandle(
      bool committed,
      bool is_same_document) {
    return content::NavigationHandle::CreateNavigationHandleForTesting(
        GURL(), main_rfh(), committed, net::OK, is_same_document);
  }

  // The lifetime of the blocking page is managed by the SSLErrorTabHelper for
  // the test's web_contents. If |denied_tracker| is passed in then it will be
  // set to true when the blocking page is denied, which happens at destruction
  // time unless it is triggered otherwise.
  SSLBlockingPage* CreateAssociatedBlockingPage(
      content::NavigationHandle* handle,
      bool* denied_tracker = nullptr) {
    SSLBlockingPage* blocking_page =
        CreateSSLBlockingPage(web_contents(), GURL(), denied_tracker);
    SSLErrorTabHelper::AssociateBlockingPage(
        web_contents(), handle->GetNavigationId(),
        std::unique_ptr<SSLBlockingPage>(blocking_page));
    return blocking_page;
  }

 private:
  // If |denied_tracker| is passed in then it will be set to true when the
  // blocking page is denied, which happens at destruction time unless it is
  // triggered otherwise.
  SSLBlockingPage* CreateSSLBlockingPage(content::WebContents* contents,
                                         GURL request_url,
                                         bool* denied_tracker) {
    net::SSLInfo ssl_info;
    ssl_info.cert =
        net::ImportCertFromFile(net::GetTestCertsDirectory(), "ok_cert.pem");
    return SSLBlockingPage::Create(
        contents, net::ERR_CERT_CONTAINS_ERRORS, ssl_info, request_url, 0,
        base::Time::NowFromSystemTime(), nullptr, false /* is superfish */,
        denied_tracker == nullptr
            ? base::Callback<void(content::CertificateRequestResultType)>()
            : base::Bind(&SSLErrorTabHelperTest::DecisionCallback,
                         denied_tracker));
  }

  static void DecisionCallback(bool* denied_tracker,
                               content::CertificateRequestResultType decision) {
    // For tests in this file, the decision should always be called from the
    // SSLBlockingPageConstructor, which denies using a decision of CANCEL.
    ASSERT_EQ(content::CERTIFICATE_REQUEST_RESULT_TYPE_CANCEL, decision);
    *denied_tracker = true;
  }
};

// Tests that the helper properly handles the lifetime of a single blocking
// page, interleaved with other navigations.
TEST_F(SSLErrorTabHelperTest, SingleBlockingPage) {
  std::unique_ptr<content::NavigationHandle> blocking_page_handle =
      CreateHandle(true, false);
  bool blocking_page_denied = false;
  SSLBlockingPage* blocking_page = CreateAssociatedBlockingPage(
      blocking_page_handle.get(), &blocking_page_denied);

  // Test that the helper knows about the blocking page.
  SSLErrorTabHelper* helper =
      SSLErrorTabHelper::FromWebContents(web_contents());
  EXPECT_FALSE(blocking_page_denied);
  EXPECT_EQ(nullptr,
            helper->GetBlockingPageForCurrentlyCommittedNavigationForTesting());
  EXPECT_EQ(1u, helper->GetBlockingPagesForNavigationsForTesting().size());
  EXPECT_EQ(
      blocking_page,
      helper
          ->GetBlockingPagesForNavigationsForTesting()[blocking_page_handle
                                                           ->GetNavigationId()]
          .get());

  // Test that a same-document navigation doesn't doens't affect the helper
  // state if the interstitial hasn't committed.
  std::unique_ptr<content::NavigationHandle> same_document_handle =
      CreateHandle(true, true);
  helper->DidFinishNavigation(same_document_handle.get());
  EXPECT_FALSE(blocking_page_denied);
  EXPECT_EQ(nullptr,
            helper->GetBlockingPageForCurrentlyCommittedNavigationForTesting());
  EXPECT_EQ(1u, helper->GetBlockingPagesForNavigationsForTesting().size());
  EXPECT_EQ(
      blocking_page,
      helper
          ->GetBlockingPagesForNavigationsForTesting()[blocking_page_handle
                                                           ->GetNavigationId()]
          .get());

  // Simulate comitting the interstitial.
  helper->DidFinishNavigation(blocking_page_handle.get());
  EXPECT_FALSE(blocking_page_denied);
  EXPECT_EQ(blocking_page,
            helper->GetBlockingPageForCurrentlyCommittedNavigationForTesting());
  EXPECT_EQ(0u, helper->GetBlockingPagesForNavigationsForTesting().size());

  // Test that a subsequent committed navigation clears the blocking page stored
  // for the currently committed navigation.
  std::unique_ptr<content::NavigationHandle> committed_handle =
      CreateHandle(true, false);
  helper->DidFinishNavigation(committed_handle.get());
  EXPECT_TRUE(blocking_page_denied);
  EXPECT_EQ(nullptr,
            helper->GetBlockingPageForCurrentlyCommittedNavigationForTesting());
  EXPECT_EQ(0u, helper->GetBlockingPagesForNavigationsForTesting().size());
}

// Tests that the helper properly handles the lifetime of multiple blocking
// pages, committed in a different order than they are created.
TEST_F(SSLErrorTabHelperTest, MultipleBlockingPages) {
  // Simulate associating and navigating to the first interstitial.
  std::unique_ptr<content::NavigationHandle> handle1 =
      CreateHandle(true, false);
  bool blocking_page1_denied = false;
  SSLBlockingPage* blocking_page1 =
      CreateAssociatedBlockingPage(handle1.get(), &blocking_page1_denied);
  SSLErrorTabHelper* helper =
      SSLErrorTabHelper::FromWebContents(web_contents());
  helper->DidFinishNavigation(handle1.get());

  // Associate the second interstitial.
  std::unique_ptr<content::NavigationHandle> handle2 =
      CreateHandle(true, false);
  bool blocking_page2_denied = false;
  SSLBlockingPage* blocking_page2 =
      CreateAssociatedBlockingPage(handle2.get(), &blocking_page2_denied);
  EXPECT_FALSE(blocking_page1_denied);
  EXPECT_FALSE(blocking_page2_denied);
  EXPECT_EQ(blocking_page1,
            helper->GetBlockingPageForCurrentlyCommittedNavigationForTesting());
  EXPECT_EQ(1u, helper->GetBlockingPagesForNavigationsForTesting().size());
  EXPECT_EQ(
      blocking_page2,
      helper
          ->GetBlockingPagesForNavigationsForTesting()[handle2
                                                           ->GetNavigationId()]
          .get());

  // Associate a third interstitial.
  std::unique_ptr<content::NavigationHandle> handle3 =
      CreateHandle(true, false);
  bool blocking_page3_denied = false;
  SSLBlockingPage* blocking_page3 =
      CreateAssociatedBlockingPage(handle3.get(), &blocking_page3_denied);
  EXPECT_FALSE(blocking_page1_denied);
  EXPECT_FALSE(blocking_page2_denied);
  EXPECT_FALSE(blocking_page3_denied);
  EXPECT_EQ(blocking_page1,
            helper->GetBlockingPageForCurrentlyCommittedNavigationForTesting());
  EXPECT_EQ(2u, helper->GetBlockingPagesForNavigationsForTesting().size());
  EXPECT_EQ(
      blocking_page2,
      helper
          ->GetBlockingPagesForNavigationsForTesting()[handle2
                                                           ->GetNavigationId()]
          .get());
  EXPECT_EQ(
      blocking_page3,
      helper
          ->GetBlockingPagesForNavigationsForTesting()[handle3
                                                           ->GetNavigationId()]
          .get());

  // Simulate commiting the third interstitial.
  helper->DidFinishNavigation(handle3.get());
  EXPECT_TRUE(blocking_page1_denied);
  EXPECT_FALSE(blocking_page2_denied);
  EXPECT_FALSE(blocking_page3_denied);
  EXPECT_EQ(blocking_page3,
            helper->GetBlockingPageForCurrentlyCommittedNavigationForTesting());
  EXPECT_EQ(1u, helper->GetBlockingPagesForNavigationsForTesting().size());
  EXPECT_EQ(
      blocking_page2,
      helper
          ->GetBlockingPagesForNavigationsForTesting()[handle2
                                                           ->GetNavigationId()]
          .get());

  // Simulate commiting the second interstitial.
  helper->DidFinishNavigation(handle2.get());
  EXPECT_TRUE(blocking_page1_denied);
  EXPECT_FALSE(blocking_page2_denied);
  EXPECT_TRUE(blocking_page3_denied);
  EXPECT_EQ(blocking_page2,
            helper->GetBlockingPageForCurrentlyCommittedNavigationForTesting());
  EXPECT_EQ(0u, helper->GetBlockingPagesForNavigationsForTesting().size());
}

}  // namespace
