// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/payment_manifest_parser_host.h"

#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/content_browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace payments {
namespace {

class PaymentManifestParserHostTest : public content::ContentBrowserTest {
 public:
  PaymentManifestParserHostTest() : all_origins_supported_(false) {}

  ~PaymentManifestParserHostTest() override {}

  void StartUtilityProcess() {
    host_.StartUtilityProcessWithName(
        base::ASCIIToUTF16("Payment Manifest Parser"));
  }

  void ParsePaymentMethodManifest(const std::string& content) {
    base::RunLoop run_loop;
    host_.ParsePaymentMethodManifest(
        content,
        base::BindOnce(
            &PaymentManifestParserHostTest::OnPaymentMethodManifestParsed,
            base::Unretained(this), run_loop.QuitClosure()));
    run_loop.Run();
  }

  const std::vector<GURL>& web_app_manifest_urls() const {
    return web_app_manifest_urls_;
  }

  const std::vector<url::Origin>& supported_origins() const {
    return supported_origins_;
  }

  bool all_origins_supported() const { return all_origins_supported_; }

 private:
  void OnPaymentMethodManifestParsed(
      const base::Closure& resume_test,
      const std::vector<GURL>& web_app_manifest_urls,
      const std::vector<url::Origin>& supported_origins,
      bool all_origins_supported) {
    web_app_manifest_urls_ = web_app_manifest_urls;
    supported_origins_ = supported_origins;
    all_origins_supported_ = all_origins_supported;
    resume_test.Run();
  }

  PaymentManifestParserHost host_;
  std::vector<GURL> web_app_manifest_urls_;
  std::vector<url::Origin> supported_origins_;
  bool all_origins_supported_;

  DISALLOW_COPY_AND_ASSIGN(PaymentManifestParserHostTest);
};

IN_PROC_BROWSER_TEST_F(PaymentManifestParserHostTest, NotStarted) {
  ParsePaymentMethodManifest("{\"supported_origins\": \"*\"}");

  EXPECT_TRUE(web_app_manifest_urls().empty());
  EXPECT_TRUE(supported_origins().empty());
  EXPECT_FALSE(all_origins_supported());
}

IN_PROC_BROWSER_TEST_F(PaymentManifestParserHostTest, AllOriginsSupported) {
  StartUtilityProcess();

  ParsePaymentMethodManifest("{\"supported_origins\": \"*\"}");

  EXPECT_TRUE(web_app_manifest_urls().empty());
  EXPECT_TRUE(supported_origins().empty());
  EXPECT_TRUE(all_origins_supported());
}

}  // namespace
}  // namespace payments
