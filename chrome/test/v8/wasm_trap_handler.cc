// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// These tests focus on Wasm out of bounds behavior to make sure trap-based
// bounds checks work when integrated with all of Chrome.

#include "base/base_switches.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"

class WasmBrowserTest : public InProcessBrowserTest {
  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(switches::kEnableCrashReporterForTesting);
    command_line->AppendSwitchNative(switches::kJavaScriptFlags,
                                     "--allow-natives-syntax");
  }

 protected:
  void RunJSTest(const std::string& js) const {
    auto* const tab = browser()->tab_strip_model()->GetActiveWebContents();
    bool result = false;

    ASSERT_TRUE(content::ExecuteScriptAndExtractBool(tab, js, &result));
    ASSERT_TRUE(result);
  }

  void RunJSTestAndEnsureTrapHandlerRan(const std::string& js) const {
    if (base::FeatureList::IsEnabled(features::kWebAssemblyTrapHandler)) {
      const auto* get_fault_count =
          "domAutomationController.send(%GetWasmRecoveredTrapCount())";
      int original_count = 0;
      auto* const tab = browser()->tab_strip_model()->GetActiveWebContents();
      ASSERT_TRUE(content::ExecuteScriptAndExtractInt(tab, get_fault_count,
                                                      &original_count));
      RunJSTest(js);
      int new_count = 0;
      ASSERT_TRUE(content::ExecuteScriptAndExtractInt(tab, get_fault_count,
                                                      &new_count));
      ASSERT_GT(new_count, original_count);
    } else {
      RunJSTest(js);
    }
  }
};

IN_PROC_BROWSER_TEST_F(WasmBrowserTest, OutOfBounds) {
  ASSERT_TRUE(embedded_test_server()->Start());
  const auto& url = embedded_test_server()->GetURL("/wasm/out_of_bounds.html");
  ui_test_utils::NavigateToURL(browser(), url);

  RunJSTest("peek_in_bounds()");
  RunJSTestAndEnsureTrapHandlerRan("peek_out_of_bounds()");
  RunJSTestAndEnsureTrapHandlerRan(
      "peek_out_of_bounds_grow_memory_from_zero_js()");
  RunJSTestAndEnsureTrapHandlerRan("peek_out_of_bounds_grow_memory_js()");
  RunJSTestAndEnsureTrapHandlerRan(
      "peek_out_of_bounds_grow_memory_from_zero_wasm()");
  RunJSTestAndEnsureTrapHandlerRan("peek_out_of_bounds_grow_memory_wasm()");

  RunJSTest("poke_in_bounds()");
  RunJSTestAndEnsureTrapHandlerRan("poke_out_of_bounds()");
  RunJSTestAndEnsureTrapHandlerRan(
      "poke_out_of_bounds_grow_memory_from_zero_js()");
  RunJSTestAndEnsureTrapHandlerRan("poke_out_of_bounds_grow_memory_js()");
  RunJSTestAndEnsureTrapHandlerRan(
      "poke_out_of_bounds_grow_memory_from_zero_wasm()");
  RunJSTestAndEnsureTrapHandlerRan("poke_out_of_bounds_grow_memory_wasm()");
}
