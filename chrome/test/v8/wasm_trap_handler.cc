// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// These tests focus on Wasm out of bounds behavior to make sure trap-based
// bounds checks work when integrated with all of Chrome.

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"

class WasmBrowserTest : public InProcessBrowserTest {
 protected:
  void RunJSTest(const std::string& js) const {
    auto* const tab = browser()->tab_strip_model()->GetActiveWebContents();
    bool result = false;

    ASSERT_TRUE(content::ExecuteScriptAndExtractBool(tab, js, &result));
    ASSERT_TRUE(result);
  }
};

IN_PROC_BROWSER_TEST_F(WasmBrowserTest, OutOfBounds) {
  ASSERT_TRUE(embedded_test_server()->Start());
  const auto& url = embedded_test_server()->GetURL("/wasm/out_of_bounds.html");
  ui_test_utils::NavigateToURL(browser(), url);

  RunJSTest("peek_in_bounds()");
  RunJSTest("peek_out_of_bounds()");
  RunJSTest("peek_out_of_bounds_grow_memory_from_zero_js()");
  RunJSTest("peek_out_of_bounds_grow_memory_js()");
  RunJSTest("peek_out_of_bounds_grow_memory_from_zero_wasm()");
  RunJSTest("peek_out_of_bounds_grow_memory_wasm()");

  RunJSTest("poke_in_bounds()");
  RunJSTest("poke_out_of_bounds()");
  RunJSTest("poke_out_of_bounds_grow_memory_from_zero_js()");
  RunJSTest("poke_out_of_bounds_grow_memory_js()");
  RunJSTest("poke_out_of_bounds_grow_memory_from_zero_wasm()");
  RunJSTest("poke_out_of_bounds_grow_memory_wasm()");
}
