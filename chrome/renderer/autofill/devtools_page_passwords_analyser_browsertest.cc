// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/renderer/devtools_page_passwords_analyser.h"

#include "chrome/test/base/chrome_render_view_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace autofill {

namespace {
const char kPasswordFormWithoutUsernameField[] =
    "<form>"
    "   <input type='password'>"
    "</form>";
}

class DevToolsPagePasswordsAnalyserTest : public ChromeRenderViewTest {
 public:
  DevToolsPagePasswordsAnalyserTest() {}

  void TearDown() override {
    devtools_page_passwords_analyser.Reset();
    ChromeRenderViewTest::TearDown();
  }

  DevToolsPagePasswordsAnalyser devtools_page_passwords_analyser;

 private:
  DISALLOW_COPY_AND_ASSIGN(DevToolsPagePasswordsAnalyserTest);
};

TEST_F(DevToolsPagePasswordsAnalyserTest, MyFirstTest) {
  LoadHTML(kPasswordFormWithoutUsernameField);
  blink::WebLocalFrame* frame = GetMainFrame();
  devtools_page_passwords_analyser.AnalyseDocumentDOM(frame);
}

}  // namespace autofill
