// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "components/language/ios/browser/language_detection_controller.h"

#include "base/mac/bind_objc_block.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "components/language/core/browser/language_detector.h"
#import "components/language/ios/browser/js_language_detection_manager.h"
#include "components/translate/core/common/language_detection_details.h"
#include "ios/web/public/test/fakes/test_web_state.h"
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

// Observer that exposes the argument with which it was called back.
class MockObserver : public language::LanguageDetector::Observer,
                     public base::SupportsWeakPtr<MockObserver> {
 public:
  void OnLanguageDetected(
      const translate::LanguageDetectionDetails& details) override {
    was_called_ = true;
    details_ = details;
  }

  bool was_called_ = false;
  translate::LanguageDetectionDetails details_;
};

// The differences between this class and the real one are:
//  1) JS injection is disabled (as it is not supported on a TestWebState)
//  2) Private methods are made public (for testing)
class TestLanguageDetectionController : public LanguageDetectionController {
 public:
  using LanguageDetectionController::OnTextCaptured;

  explicit TestLanguageDetectionController(
      web::WebState* const web_state,
      JsLanguageDetectionManager* const js_manager)
      : LanguageDetectionController(web_state, js_manager) {}
};

class LanguageDetectionControllerTest : public PlatformTest {
 public:
  LanguageDetectionControllerTest() {
    MockJsLanguageDetectionManager* js_manager =
        [[MockJsLanguageDetectionManager alloc] init];
    controller_ = base::MakeUnique<TestLanguageDetectionController>(&web_state_,
                                                                    js_manager);
  }

  TestLanguageDetectionController* controller() { return controller_.get(); }

 private:
  web::TestWebState web_state_;
  std::unique_ptr<TestLanguageDetectionController> controller_;
};

// Tests that OnTextCaptured() correctly handles messages from the JS side and
// informs the driver.
TEST_F(LanguageDetectionControllerTest, OnTextCaptured) {
  const std::string kRootLanguage("en");
  const std::string kContentLanguage("fr");
  const std::string kUndefined("und");

  MockObserver ob;
  controller()->AddLanguageDetectionObserver(ob.AsWeakPtr());

  base::DictionaryValue command;
  command.SetString("command", "languageDetection.textCaptured");
  command.SetBoolean("translationAllowed", true);
  command.SetInteger("captureTextTime", 10);
  command.SetString("htmlLang", kRootLanguage);
  command.SetString("httpContentLanguage", kContentLanguage);

  controller()->OnTextCaptured(command, GURL("http://google.com"), false);

  ASSERT_TRUE(ob.was_called_);
  EXPECT_EQ(kRootLanguage, ob.details_.html_root_language);
  EXPECT_EQ(kContentLanguage, ob.details_.content_language);
  EXPECT_FALSE(ob.details_.is_cld_reliable);
  EXPECT_EQ(kUndefined, ob.details_.cld_language);
}

}
