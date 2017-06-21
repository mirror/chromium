// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/payment_manifest_parser_host.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace payments {

// Test fixture for payment manifest parser host.
class PaymentManifestParserHostTest : public InProcessBrowserTest {
 public:
  PaymentManifestParserHostTest() : all_origins_supported_(false) {}
  ~PaymentManifestParserHostTest() override {}

  // Starts the utility process. If this is not called, all parse requests fail.
  void StartUtilityProcess() { host_.StartUtilityProcess(); }

  // Sends the |content| to the utility process to parse as a web app manifest
  // and waits until the utility process responds.
  void ParseWebAppManifest(const std::string& content) {
    base::RunLoop run_loop;
    host_.ParseWebAppManifest(
        content,
        base::BindOnce(&PaymentManifestParserHostTest::OnWebAppManifestParsed,
                       base::Unretained(this), run_loop.QuitClosure()));
    run_loop.Run();
  }

  // Sends the |content| to the utility process to parse as a payment method
  // manifest and waits until the utility process responds.
  void ParsePaymentMethodManifest(const std::string& content) {
    base::RunLoop run_loop;
    host_.ParsePaymentMethodManifest(
        content,
        base::BindOnce(
            &PaymentManifestParserHostTest::OnPaymentMethodManifestParsed,
            base::Unretained(this), run_loop.QuitClosure()));
    run_loop.Run();
  }

  // The parsed web app manifest.
  const std::vector<mojom::WebAppManifestSectionPtr>& web_app_manifest() const {
    return web_app_manifest_;
  }

  // The parsed web app manifest URLs from the payment method manifest.
  const std::vector<GURL>& web_app_manifest_urls() const {
    return web_app_manifest_urls_;
  }

  // The parsed supported origins from the payment method manifest.
  const std::vector<url::Origin>& supported_origins() const {
    return supported_origins_;
  }

  // Whether the parsed payment method manifest allows all origins to use the
  // payment method.
  bool all_origins_supported() const { return all_origins_supported_; }

  // The utility process host.
  PaymentManifestParserHost* host() { return &host_; }

 private:
  // Called after the utility process has parsed the web app manifest.
  void OnWebAppManifestParsed(
      const base::Closure& resume_test,
      std::vector<mojom::WebAppManifestSectionPtr> web_app_manifest) {
    web_app_manifest_ = std::move(web_app_manifest);
    resume_test.Run();
  }

  // Called after the utility process has parsed the payment method manifest.
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
  std::vector<mojom::WebAppManifestSectionPtr> web_app_manifest_;
  std::vector<GURL> web_app_manifest_urls_;
  std::vector<url::Origin> supported_origins_;
  bool all_origins_supported_;

  DISALLOW_COPY_AND_ASSIGN(PaymentManifestParserHostTest);
};

// If the utility process has not been started, parsing a payment method
// manifest should fail gracefully.
IN_PROC_BROWSER_TEST_F(PaymentManifestParserHostTest,
                       NotStarted_PaymentMethodManifest) {
  ParsePaymentMethodManifest("{\"supported_origins\": \"*\"}");

  EXPECT_TRUE(web_app_manifest_urls().empty());
  EXPECT_TRUE(supported_origins().empty());
  EXPECT_FALSE(all_origins_supported());
}

// Handle gracefully a utility process that returns a result unexpectedly.
IN_PROC_BROWSER_TEST_F(PaymentManifestParserHostTest,
                       UnexpectedOnPaymentMethodParse) {
  host()->OnPaymentMethodParse(0, std::vector<GURL>(),
                               std::vector<url::Origin>(), true);

  EXPECT_FALSE(all_origins_supported());
}

IN_PROC_BROWSER_TEST_F(PaymentManifestParserHostTest, AllOriginsSupported) {
  StartUtilityProcess();

  ParsePaymentMethodManifest("{\"supported_origins\": \"*\"}");

  EXPECT_TRUE(web_app_manifest_urls().empty());
  EXPECT_TRUE(supported_origins().empty());
  EXPECT_TRUE(all_origins_supported());
}

IN_PROC_BROWSER_TEST_F(PaymentManifestParserHostTest, UrlsAndOrigins) {
  StartUtilityProcess();

  ParsePaymentMethodManifest(
      "{\"default_applications\": "
      "[\"https://alicepay.com/web-app-manifest.json\"], "
      "\"supported_origins\": [\"https://bobpay.com\"]}");

  EXPECT_EQ(
      std::vector<GURL>(1, GURL("https://alicepay.com/web-app-manifest.json")),
      web_app_manifest_urls());
  EXPECT_EQ(
      std::vector<url::Origin>(1, url::Origin(GURL("https://bobpay.com"))),
      supported_origins());
  EXPECT_FALSE(all_origins_supported());
}

// If the utility process has not been started, parsing a web app manifest
// should fail gracefully.
IN_PROC_BROWSER_TEST_F(PaymentManifestParserHostTest,
                       NotStarted_WebAppManifest) {
  ParseWebAppManifest(
      "{"
      "  \"related_applications\": [{"
      "    \"platform\": \"play\", "
      "    \"id\": \"com.bobpay.app\", "
      "    \"min_version\": \"1\", "
      "    \"fingerprints\": [{"
      "      \"type\": \"sha256_cert\", "
      "      \"value\": \"00:01:02:03:04:05:06:07:08:09:A0:A1:A2:A3:A4:A5:A6:A7"
      ":A8:A9:B0:B1:B2:B3:B4:B5:B6:B7:B8:B9:C0:C1\""
      "    }]"
      "  }]"
      "}");

  EXPECT_TRUE(web_app_manifest().empty());
}

// Handle gracefully a utility process that returns a result unexpectedly.
IN_PROC_BROWSER_TEST_F(PaymentManifestParserHostTest, UnexpectedOnWebAppParse) {
  std::vector<mojom::WebAppManifestSectionPtr> manifest;
  manifest.push_back(mojom::WebAppManifestSection::New());

  host()->OnWebAppParse(0, std::move(manifest));

  EXPECT_TRUE(web_app_manifest().empty());
}

IN_PROC_BROWSER_TEST_F(PaymentManifestParserHostTest, WebAppManifest) {
  StartUtilityProcess();

  ParseWebAppManifest(
      "{"
      "  \"related_applications\": [{"
      "    \"platform\": \"play\", "
      "    \"id\": \"com.bobpay.app\", "
      "    \"min_version\": \"1\", "
      "    \"fingerprints\": [{"
      "      \"type\": \"sha256_cert\", "
      "      \"value\": \"00:01:02:03:04:05:06:07:08:09:A0:A1:A2:A3:A4:A5:A6:A7"
      ":A8:A9:B0:B1:B2:B3:B4:B5:B6:B7:B8:B9:C0:C1\""
      "    }]"
      "  }]"
      "}");

  ASSERT_EQ(1U, web_app_manifest().size());
  EXPECT_EQ("com.bobpay.app", web_app_manifest().front()->id);
  EXPECT_EQ(1, web_app_manifest().front()->min_version);
  ASSERT_EQ(1U, web_app_manifest().front()->fingerprints.size());
  EXPECT_EQ(
      std::vector<uint8_t>({0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                            0x08, 0x09, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5,
                            0xA6, 0xA7, 0xA8, 0xA9, 0xB0, 0xB1, 0xB2, 0xB3,
                            0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xC0, 0xC1}),
      web_app_manifest().front()->fingerprints.front());
}

}  // namespace payments
