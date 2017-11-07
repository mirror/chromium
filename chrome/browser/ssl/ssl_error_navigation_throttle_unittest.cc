// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/ssl_error_navigation_throttle.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "chrome/browser/ssl/certificate_reporting_test_utils.cc"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle.h"
#include "content/public/test/test_utils.h"
#include "net/base/net_errors.h"
#include "net/cert/cert_status_flags.h"
#include "net/test/cert_test_util.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/test_data_directory.h"

namespace {

class TestSSLErrorNavigationThrottle : public SSLErrorNavigationThrottle {
 public:
  explicit TestSSLErrorNavigationThrottle(content::NavigationHandle* handle)
      : SSLErrorNavigationThrottle(
            handle,
            certificate_reporting_test_utils::CreateMockSSLCertReporter(
                base::Callback<void(const std::string&,
                                    const certificate_reporting::
                                        CertLoggerRequest_ChromeChannel)>(),
                certificate_reporting_test_utils::CERT_REPORT_NOT_EXPECTED)) {}
};

class SSLErrorNavigationThrottleTest : public ChromeRenderViewHostTestHarness {
 public:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kCommittedInterstitials);

    handle_ = content::NavigationHandle::CreateNavigationHandleForTesting(
        GURL(), main_rfh(), true /* committed */, net::ERR_CERT_INVALID);
    throttle_ = std::make_unique<TestSSLErrorNavigationThrottle>(handle_.get());
  }

  void TearDown() override { handle_.reset(); }

  std::unique_ptr<content::NavigationHandle> handle_;
  std::unique_ptr<TestSSLErrorNavigationThrottle> throttle_;
};

// Tests that the throttle ignores a request without SSL info, by returning
// PROCEED synchronously.
TEST_F(SSLErrorNavigationThrottleTest, NoSSLInfo) {
  content::NavigationThrottle::ThrottleCheckResult result =
      handle_->CallWillFailRequestForTesting(
          base::nullopt, false /* should_ssl_errors_be_fatal */);

  EXPECT_FALSE(handle_->GetSSLInfo().has_value());
  EXPECT_EQ(content::NavigationThrottle::PROCEED, result);
}

// Tests that the throttle ignores a request with a cert status that is not an
// cert error, by returning PROCEED synchronously.
TEST_F(SSLErrorNavigationThrottleTest, SSLInfoWithoutCertError) {
  net::SSLInfo ssl_info;
  ssl_info.cert_status = net::CERT_STATUS_IS_EV;
  content::NavigationThrottle::ThrottleCheckResult result =
      handle_->CallWillFailRequestForTesting(
          ssl_info, false /* should_ssl_errors_be_fatal */);

  ASSERT_TRUE(handle_->GetSSLInfo().has_value());
  EXPECT_EQ(net::CERT_STATUS_IS_EV, handle_->GetSSLInfo().value().cert_status);
  EXPECT_EQ(content::NavigationThrottle::PROCEED, result);
}

}  // namespace
