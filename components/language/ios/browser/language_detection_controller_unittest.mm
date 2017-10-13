// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "components/language/ios/browser/language_detection_controller.h"

#include "base/mac/bind_objc_block.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#import "components/language/ios/browser/ios_language_detection_client.h"
#import "components/language/ios/browser/ios_language_detection_driver.h"
#import "components/language/ios/browser/js_language_detection_manager.h"
#include "components/translate/core/common/language_detection_details.h"
#import "ios/web/public/test/fakes/fake_navigation_context.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "net/http/http_response_headers.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface MockJsLanguageDetectionManager : JsLanguageDetectionManager
@end

@implementation MockJsLanguageDetectionManager
- (void)retrieveBufferedTextContent:
    (const language_detection::BufferedTextCallback&)callback {
  callback.Run(base::UTF8ToUTF16("Some content"));
}
@end

namespace language {

// Driver that doesn't provide access to either the language histogram or
// translate.
class MockLanguageDetectionDriver : public IOSLanguageDetectionDriver {
 public:
  UrlLanguageHistogram* GetUrlLanguageHistogram() override { return nullptr; }
  translate::IOSTranslateDriver* GetTranslateDriver() override {
    return nullptr;
  }
};

class LanguageDetectionControllerTest : public PlatformTest {
 protected:
  LanguageDetectionControllerTest()
      : PlatformTest(),
        web_state_(),
        controller_(&web_state_,
                    [[MockJsLanguageDetectionManager alloc] init]) {
    // Create a language detection client with a mock driver.
    IOSLanguageDetectionClient::CreateForWebState(&web_state_);
    IOSLanguageDetectionClient::FromWebState(&web_state_)
        ->set_driver(base::MakeUnique<MockLanguageDetectionDriver>());
  }

  IOSLanguageDetectionClient& client() {
    return *IOSLanguageDetectionClient::FromWebState(&web_state_);
  }
  LanguageDetectionController& controller() { return controller_; }
  web::TestWebState& web_state() { return web_state_; }

 private:
  web::TestWebState web_state_;
  LanguageDetectionController controller_;
};

// Tests that OnTextCaptured() correctly handles messages from the JS side and
// informs the driver.
TEST_F(LanguageDetectionControllerTest, OnTextCaptured) {
  const std::string kRootLanguage("en");
  const std::string kContentLanguage("fr");
  const std::string kUndefined("und");

  __block bool block_was_called = false;
  client().set_callback_for_testing(
      base::BindBlockArc(^(const translate::LanguageDetectionDetails& details) {
        block_was_called = true;
        EXPECT_EQ(kRootLanguage, details.html_root_language);
        EXPECT_EQ(kContentLanguage, details.content_language);
        EXPECT_FALSE(details.is_cld_reliable);
        EXPECT_EQ(kUndefined, details.cld_language);
      }));

  base::DictionaryValue command;
  command.SetString("command", "languageDetection.textCaptured");
  command.SetBoolean("translationAllowed", true);
  command.SetInteger("captureTextTime", 10);
  command.SetString("htmlLang", kRootLanguage);
  command.SetString("httpContentLanguage", kContentLanguage);
  controller().OnTextCaptured(command, GURL("http://google.com"), false);

  EXPECT_TRUE(block_was_called);
}

// Tests that Content-Language response header is used if httpContentLanguage
// message value is empty.
TEST_F(LanguageDetectionControllerTest, MissingHttpContentLanguage) {
  // Pass content-language header to LanguageDetectionController.
  scoped_refptr<net::HttpResponseHeaders> headers(
      new net::HttpResponseHeaders(""));
  headers->AddHeader("Content-Language: fr, en-CA");
  web::FakeNavigationContext context;
  context.SetResponseHeaders(headers);
  web_state().OnNavigationFinished(&context);

  __block bool block_was_called = false;
  client().set_callback_for_testing(
      base::BindBlockArc(^(const translate::LanguageDetectionDetails& details) {
        block_was_called = true;
        EXPECT_EQ("fr", details.content_language);
      }));

  base::DictionaryValue command;
  command.SetString("command", "languageDetection.textCaptured");
  command.SetBoolean("translationAllowed", true);
  command.SetInteger("captureTextTime", 10);
  command.SetString("htmlLang", "");
  command.SetString("httpContentLanguage", "");
  controller().OnTextCaptured(command, GURL("http://google.com"), false);

  EXPECT_TRUE(block_was_called);
}

}  // namespace language
