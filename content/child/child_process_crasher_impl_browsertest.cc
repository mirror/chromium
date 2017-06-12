// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/render_process_host.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

class ChildProcessCrasherBrowserTest : public ContentBrowserTest {
 public:
  ChildProcessCrasherBrowserTest() {}
  ~ChildProcessCrasherBrowserTest() override {}
};

IN_PROC_BROWSER_TEST_F(ChildProcessCrasherBrowserTest, Crash) {
  NavigateToURL(shell(), GURL("about:blank"));
  RenderProcessHost* rph = shell()->web_contents()->GetRenderProcessHost();
  RenderProcessHostWatcher watcher(
      rph, RenderProcessHostWatcher::WATCH_FOR_PROCESS_EXIT);
  rph->TerminateHungRenderProcess();
  watcher.Wait();
}

}  // namespace content
