// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/payments/ios_payment_app_finder.h"

#include "base/memory/ptr_util.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace payments {
namespace {

class IOSPaymentAppFinderTest : public testing::Test {
 public:
  IOSPaymentAppFinderTest()
      : ios_payment_app_finder_(base::MakeUnique<IOSPaymentAppFinder>(
            base::Bind(&IOSPaymentAppFinderTest::OnPaymentAppDetailsReceieved,
                       base::Unretained(this)),
            new net::TestURLRequestContextGetter(
                base::ThreadTaskRunnerHandle::Get()))) {}

  ~IOSPaymentAppFinderTest() override {}

  MOCK_METHOD3(OnPaymentAppDetailsReceieved,
               void(std::string& app_name,
                    std::string& app_icon,
                    std::string& universal_link));

  void ExpectUnableToParsePaymentMethodManifest(const std::string& input) {
    current_web_app_url_.reset(new std::string(""));

    bool success =
        ios_payment_app_finder_->ParsePaymentManifestForWebAppManifestURL(
            input, *current_web_app_url_);

    EXPECT_FALSE(success);
    EXPECT_TRUE(current_web_app_url_->empty());
  }

  void ExpectParsedPaymentMethodManifest(
      const std::string& input,
      const std::string& expected_web_app_url) {
    current_web_app_url_.reset(new std::string(""));

    bool success =
        ios_payment_app_finder_->ParsePaymentManifestForWebAppManifestURL(
            input, *current_web_app_url_);

    EXPECT_TRUE(success);
    EXPECT_EQ(expected_web_app_url, *current_web_app_url_);
  }

  void ExpectUnableToParseWebAppManifest(const std::string& input) {
    current_app_name_.reset(new std::string(""));
    current_app_icon_.reset(new std::string(""));
    current_unviersal_link_.reset(new std::string(""));

    bool success =
        ios_payment_app_finder_->ParseWebAppManifestForPaymentAppDetails(
            input, *current_app_name_, *current_app_icon_,
            *current_unviersal_link_);

    EXPECT_FALSE(success);
    EXPECT_TRUE(current_app_name_->empty() || current_app_icon_->empty() ||
                current_unviersal_link_->empty());
  }

  void ExpectParsedWebAppManifest(const std::string& input,
                                  const std::string& expected_app_name,
                                  const std::string& expected_app_icon,
                                  const std::string& expected_universal_link) {
    current_app_name_.reset(new std::string(""));
    current_app_icon_.reset(new std::string(""));
    current_unviersal_link_.reset(new std::string(""));
    ;
    bool success =
        ios_payment_app_finder_->ParseWebAppManifestForPaymentAppDetails(
            input, *current_app_name_, *current_app_icon_,
            *current_unviersal_link_);
    EXPECT_TRUE(success);
    EXPECT_EQ(expected_app_name, *current_app_name_);
    EXPECT_EQ(expected_app_icon, *current_app_icon_);
    EXPECT_EQ(expected_universal_link, *current_unviersal_link_);
  }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  std::unique_ptr<std::string> current_web_app_url_;
  std::unique_ptr<std::string> current_app_name_;
  std::unique_ptr<std::string> current_app_icon_;
  std::unique_ptr<std::string> current_unviersal_link_;

  std::unique_ptr<IOSPaymentAppFinder> ios_payment_app_finder_;

  DISALLOW_COPY_AND_ASSIGN(IOSPaymentAppFinderTest);
};

// Payment method manifest parsing:

TEST_F(IOSPaymentAppFinderTest, NullPaymentMethodManifestIsMalformed) {
  ExpectUnableToParsePaymentMethodManifest(std::string());
}

TEST_F(IOSPaymentAppFinderTest, NonJsonPaymentMethodManifestIsMalformed) {
  ExpectUnableToParsePaymentMethodManifest("this is not json");
}

TEST_F(IOSPaymentAppFinderTest, StringPaymentMethodManifestIsMalformed) {
  ExpectUnableToParsePaymentMethodManifest("\"this is a string\"");
}

TEST_F(IOSPaymentAppFinderTest,
       EmptyDictionaryPaymentMethodManifestIsMalformed) {
  ExpectUnableToParsePaymentMethodManifest("{}");
}

TEST_F(IOSPaymentAppFinderTest, NullDefaultApplicationIsMalformed) {
  ExpectUnableToParsePaymentMethodManifest("{\"default_applications\": null}");
}

TEST_F(IOSPaymentAppFinderTest, NumberDefaultApplicationIsMalformed) {
  ExpectUnableToParsePaymentMethodManifest("{\"default_applications\": 0}");
}

TEST_F(IOSPaymentAppFinderTest, ListOfNumbersDefaultApplicationIsMalformed) {
  ExpectUnableToParsePaymentMethodManifest("{\"default_applications\": [0]}");
}

TEST_F(IOSPaymentAppFinderTest, EmptyListOfDefaultApplicationsIsMalformed) {
  ExpectUnableToParsePaymentMethodManifest("{\"default_applications\": []}");
}

TEST_F(IOSPaymentAppFinderTest, ListOfEmptyDefaultApplicationsIsMalformed) {
  ExpectUnableToParsePaymentMethodManifest(
      "{\"default_applications\": [\"\"]}");
}

TEST_F(IOSPaymentAppFinderTest, DefaultApplicationsShouldNotHaveNulCharacters) {
  ExpectUnableToParsePaymentMethodManifest(
      "{\"default_applications\": [\"https://bobpay.com/app\0json\"]}");
}

TEST_F(IOSPaymentAppFinderTest, DefaultApplicationKeyShouldBeLowercase) {
  ExpectUnableToParsePaymentMethodManifest(
      "{\"Default_Applications\": [\"https://bobpay.com/app.json\"]}");
}

TEST_F(IOSPaymentAppFinderTest, DefaultApplicationsShouldHaveAbsoluteUrl) {
  ExpectUnableToParsePaymentMethodManifest(
      "{\"default_applications\": ["
      "\"app.json\"]}");
}

TEST_F(IOSPaymentAppFinderTest, DefaultApplicationsShouldBeHttps) {
  ExpectUnableToParsePaymentMethodManifest(
      "{\"default_applications\": ["
      "\"http://bobpay.com/app.json\","
      "\"http://alicepay.com/app.json\"]}");
}

TEST_F(IOSPaymentAppFinderTest, WellFormedPaymentMethodManifestWithApps) {
  ExpectParsedPaymentMethodManifest(
      "{\"default_applications\": ["
      "\"https://bobpay.com/app.json\","
      "\"https://alicepay.com/app.json\"]}",
      "https://bobpay.com/app.json");
}

// Web app manifest parsing:

TEST_F(IOSPaymentAppFinderTest, NullContentIsMalformed) {
  ExpectUnableToParseWebAppManifest(std::string());
}

TEST_F(IOSPaymentAppFinderTest, NonJsonContentIsMalformed) {
  ExpectUnableToParseWebAppManifest("this is not json");
}

TEST_F(IOSPaymentAppFinderTest, StringContentIsMalformed) {
  ExpectUnableToParseWebAppManifest("\"this is a string\"");
}

TEST_F(IOSPaymentAppFinderTest, EmptyDictionaryIsMalformed) {
  ExpectUnableToParseWebAppManifest("{}");
}

TEST_F(IOSPaymentAppFinderTest, NullRelatedApplicationsSectionIsMalformed) {
  ExpectUnableToParseWebAppManifest(
      "{"
      "  \"short_name\": \"Bobpay\", "
      "  \"icons\": [{"
      "    \"src\": \"images/touch/homescreen48.png\""
      "  }], "
      "  \"related_applications\": null"
      "}");
}

TEST_F(IOSPaymentAppFinderTest, NumberRelatedApplicationSectionIsMalformed) {
  ExpectUnableToParseWebAppManifest(
      "{"
      "  \"short_name\": \"Bobpay\", "
      "  \"icons\": [{"
      "    \"src\": \"images/touch/homescreen48.png\""
      "  }], "
      "  \"related_applications\": 0"
      "}");
}

TEST_F(IOSPaymentAppFinderTest,
       ListOfNumbersRelatedApplicationsSectionIsMalformed) {
  ExpectUnableToParseWebAppManifest(
      "{"
      "  \"short_name\": \"Bobpay\", "
      "  \"icons\": [{"
      "    \"src\": \"images/touch/homescreen48.png\""
      "  }], "
      "  \"related_applications\": [0]"
      "}");
}

TEST_F(IOSPaymentAppFinderTest,
       EmptyListRelatedApplicationsSectionIsMalformed) {
  ExpectUnableToParseWebAppManifest(
      "{"
      "  \"short_name\": \"Bobpay\", "
      "  \"icons\": [{"
      "    \"src\": \"images/touch/homescreen48.png\""
      "  }], "
      "  \"related_applications\": []"
      "}");
}

TEST_F(IOSPaymentAppFinderTest,
       ListOfEmptyDictionariesRelatedApplicationsSectionIsMalformed) {
  ExpectUnableToParseWebAppManifest(
      "{"
      "  \"short_name\": \"Bobpay\", "
      "  \"icons\": [{"
      "    \"src\": \"images/touch/homescreen48.png\""
      "  }], "
      "  \"related_applications\": [{}]"
      "}");
}

TEST_F(IOSPaymentAppFinderTest, NoItunesPlatformIsMalformed) {
  ExpectUnableToParseWebAppManifest(
      "{"
      "  \"short_name\": \"Bobpay\", "
      "  \"icons\": [{"
      "    \"src\": \"images/touch/homescreen48.png\""
      "  }], "
      "  \"related_applications\": [{"
      "    \"id\": \"https://bobpay.xyz/pay\""
      "  }]"
      "}");
}

TEST_F(IOSPaymentAppFinderTest, NoUniversalLinkIsMalformed) {
  ExpectUnableToParseWebAppManifest(
      "{"
      "  \"short_name\": \"Bobpay\", "
      "  \"icons\": [{"
      "    \"src\": \"images/touch/homescreen48.png\""
      "  }], "
      "  \"related_applications\": [{"
      "    \"platform\": \"itunes\""
      "  }]"
      "}");
}

TEST_F(IOSPaymentAppFinderTest, NoShortNameIsMalformed) {
  ExpectUnableToParseWebAppManifest(
      "{"
      "  \"icons\": [{"
      "    \"src\": \"images/touch/homescreen48.png\""
      "  }], "
      "  \"related_applications\": [{"
      "    \"platform\": \"itunes\""
      "  }]"
      "}");
}

TEST_F(IOSPaymentAppFinderTest, PlatformShouldNotHaveNullCharacters) {
  ExpectUnableToParseWebAppManifest(
      "{"
      "  \"short_name\": \"Bobpay\", "
      "  \"icons\": [{"
      "    \"src\": \"images/touch/homescreen48.png\""
      "  }], "
      "  \"related_applications\": [{"
      "    \"platform\": \"it\0unes\", "
      "    \"id\": \"https://bobpay.xyz/pay\""
      "  }]"
      "}");
}

TEST_F(IOSPaymentAppFinderTest, UniversalLinkShouldNotHaveNullCharacters) {
  ExpectUnableToParseWebAppManifest(
      "{"
      "  \"short_name\": \"Bobpay\", "
      "  \"icons\": [{"
      "    \"src\": \"images/touch/homescreen48.png\""
      "  }], "
      "  \"related_applications\": [{"
      "    \"platform\": \"itunes\", "
      "    \"id\": \"https://bobp\0ay.xyz/pay\""
      "  }]"
      "}");
}

TEST_F(IOSPaymentAppFinderTest, IconSourceShouldNotHaveNullCharacters) {
  ExpectUnableToParseWebAppManifest(
      "{"
      "  \"short_name\": \"Bobpay\", "
      "  \"icons\": [{"
      "    \"src\": \"images/to\0uch/homescreen48.png\""
      "  }], "
      "  \"related_applications\": [{"
      "    \"platform\": \"itunes\", "
      "    \"id\": \"https://bobpay.xyz/pay\""
      "  }]"
      "}");
}

TEST_F(IOSPaymentAppFinderTest, ShortNameShouldNotHaveNullCharacters) {
  ExpectUnableToParseWebAppManifest(
      "{"
      "  \"short_name\": \"Bob\0pay\", "
      "  \"icons\": [{"
      "    \"src\": \"images/touch/homescreen48.png\""
      "  }], "
      "  \"related_applications\": [{"
      "    \"platform\": \"itunes\", "
      "    \"id\": \"https://bobpay.xyz/pay\""
      "  }]"
      "}");
}

TEST_F(IOSPaymentAppFinderTest, KeysShouldBeLowerCase) {
  ExpectUnableToParseWebAppManifest(
      "{"
      "  \"Short_name\": \"Bobpay\", "
      "  \"Icons\": [{"
      "    \"Src\": \"images/touch/homescreen48.png\""
      "  }], "
      "  \"Related_applications\": [{"
      "    \"Platform\": \"itunes\", "
      "    \"Id\": \"https://bobpay.xyz/pay\""
      "  }]"
      "}");
}

TEST_F(IOSPaymentAppFinderTest, UniversalLinkShouldHaveAbsoluteUrl) {
  ExpectUnableToParsePaymentMethodManifest(
      "{"
      "  \"short_name\": \"Bobpay\", "
      "  \"icons\": [{"
      "    \"src\": \"images/touch/homescreen48.png\""
      "  }], "
      "  \"related_applications\": [{"
      "    \"platform\": \"itunes\", "
      "    \"id\": \"pay.html\""
      "  }]"
      "}");
}

TEST_F(IOSPaymentAppFinderTest, UniversalLinkShouldBeHttps) {
  ExpectUnableToParsePaymentMethodManifest(
      "{"
      "  \"short_name\": \"Bobpay\", "
      "  \"icons\": [{"
      "    \"src\": \"images/touch/homescreen48.png\""
      "  }], "
      "  \"related_applications\": [{"
      "    \"platform\": \"itunes\", "
      "    \"id\": \"http://bobpay.xyz/pay\""
      "  }]"
      "}");
}

TEST_F(IOSPaymentAppFinderTest, WellFormed) {
  ExpectParsedWebAppManifest(
      "{"
      "  \"short_name\": \"Bobpay\", "
      "  \"icons\": [{"
      "    \"src\": \"images/touch/homescreen48.png\""
      "  }], "
      "  \"related_applications\": [{"
      "    \"platform\": \"itunes\", "
      "    \"id\": \"https://bobpay.xyz/pay\""
      "  }]"
      "}",
      "Bobpay", "images/touch/homescreen48.png", "https://bobpay.xyz/pay");
}

TEST_F(IOSPaymentAppFinderTest, TwoRelatedApplicationsSecondIsWellFormed) {
  ExpectParsedWebAppManifest(
      "{"
      "  \"short_name\": \"Bobpay\", "
      "  \"icons\": [{"
      "    \"src\": \"images/touch/homescreen48.png\""
      "  }], "
      "  \"related_applications\": [{"
      "    \"platform\": \"play\", "
      "    \"id\": \"https://bobpay.xyz/pay\""
      "  }, {"
      "    \"platform\": \"itunes\", "
      "    \"id\": \"https://bobpay.xyz/pay\""
      "  }]"
      "}",
      "Bobpay", "images/touch/homescreen48.png", "https://bobpay.xyz/pay");
}

}  // namespace
}  // namespace payments
