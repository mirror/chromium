// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/spellchecker/spell_check_host_chrome_impl.h"

#include <memory>
#include <tuple>
#include <vector>

#include "base/command_line.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/spellcheck/common/spellcheck_result.h"
#include "testing/gtest/include/gtest/gtest.h"

typedef InProcessBrowserTest SpellCheckHostChromeImplMacBrowserTest;

void GetSpellCheckResult(std::vector<SpellCheckResult>* target,
                         base::OnceClusure quit,
                         const std::vector<SpellCheckResult>& source) {
  *target = source;
  std::move(quit).Run();
}

// Uses browsertest to setup chrome threads.
IN_PROC_BROWSER_TEST_F(SpellCheckHostChromeImplMacBrowserTest,
                       SpellCheckReturnMessage) {
  // TODO(xiaochengh): How to set this up correctly?
  SpellCheckHostChromeImpl spell_check_host;

  base::RunLoop run_loop;
  base::OnceClosure quit = run_loop.QuitClosure();

  std::vector<SpellCheckResult> results;
  spell_check_host.RequestTextCheck(
      base::UTF8ToUTF16("zz."),
      base::BindOnce(&GetSpellCheckResult, &results, std::move(quit)));
  run_loop.Run();

  ASSERT_EQ(1U, results.size());
  EXPECT_EQ(results[0].location, 0);
  EXPECT_EQ(results[0].length, 2);
  EXPECT_EQ(results[0].decoration, SpellCheckResult::SPELLING);
}
