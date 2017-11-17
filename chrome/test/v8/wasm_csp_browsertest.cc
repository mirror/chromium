// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// These tests verify that our wasm-eval CSP directive is correctly handled.
//
// It would probably be easier and better to add these tests to WPT, but
// currently wasm-eval is a Chrome-only feature. If wasm-eval is adopted as a
// web standard, then these tests should be migrated to WPT.

#include "base/base_switches.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"

namespace {
constexpr bool kExpectPass = true;
constexpr bool kExpectFail = false;

class WasmCspBrowserTest : public InProcessBrowserTest {
 public:
  WasmCspBrowserTest() {}
  ~WasmCspBrowserTest() override {}

 protected:
  void RunJSTest(const std::string& js, bool expect_pass) const {
    auto* const tab = browser()->tab_strip_model()->GetActiveWebContents();
    bool result = false;

    ASSERT_TRUE(content::ExecuteScriptAndExtractBool(tab, js, &result));
    ASSERT_EQ(result, expect_pass);
  }

  void TestInstantiateCSP(const std::string& page, bool expect_pass) const {
    const auto& url = embedded_test_server()->GetURL(page);
    ui_test_utils::NavigateToURL(browser(), url);

    ASSERT_NO_FATAL_FAILURE(RunJSTest("try_instantiate()", expect_pass));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(WasmCspBrowserTest);
};

#define WASM_CSP_TEST(name, page, expect_pass)       \
  IN_PROC_BROWSER_TEST_F(WasmCspBrowserTest, name) { \
    ASSERT_TRUE(embedded_test_server()->Start());    \
    TestInstantiateCSP(page, expect_pass);           \
  }

WASM_CSP_TEST(Unspecified, "/wasm/csp_unspecified.html", kExpectPass);
WASM_CSP_TEST(Default, "/wasm/csp_default.html", kExpectFail);
WASM_CSP_TEST(UnsafeEval, "/wasm/csp_unsafe_eval.html", kExpectPass);
// wasm-eval is currently a non-standard CSP, so we don't want this to work on
// the open web.
WASM_CSP_TEST(WasmEval, "/wasm/csp_wasm_eval.html", kExpectFail);

}  // namespace
