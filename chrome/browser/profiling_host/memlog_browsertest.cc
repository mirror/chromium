// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/allocator/features.h"
// #include "base/memory/ref_counted_memory.h"
#include "base/threading/thread_restrictions.h"
#include "base/trace_event/trace_buffer.h"
#include "base/trace_event/trace_log.h"
#include "build/build_config.h"
#include "chrome/browser/profiling_host/profiling_process_host.h"
#include "chrome/browser/profiling_host/profiling_test_driver.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/zlib/zlib.h"

// Some builds don't support the allocator shim in which case the memory long
// won't function.
#if BUILDFLAG(USE_ALLOCATOR_SHIM)

namespace {


class MemlogBrowserTest : public InProcessBrowserTest,
                          public testing::WithParamInterface<const char*> {
 protected:
  void SetUpDefaultCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpDefaultCommandLine(command_line);
    if (GetParam())
      command_line->AppendSwitchASCII(switches::kMemlog, GetParam());
  }

};

IN_PROC_BROWSER_TEST_P(MemlogBrowserTest, EndToEndTracing) {
  profiling::ProfilingTestDriver driver;
  profiling::ProfilingTestDriver::Options options;
  options.mode =
      GetParam()
          ? profiling::ProfilingProcessHost::ConvertStringToMode(GetParam())
          : profiling::ProfilingProcessHost::Mode::kNone;
  options.profiling_already_started = true;

  EXPECT_TRUE(driver.RunTest(options));
}

// TODO(ajwong): Test what happens if profiling process crashes.
// http://crbug.com/780955

INSTANTIATE_TEST_CASE_P(NoMemlog,
                        MemlogBrowserTest,
                        ::testing::Values(static_cast<const char*>(nullptr)));
INSTANTIATE_TEST_CASE_P(Minimal,
                        MemlogBrowserTest,
                        ::testing::Values(switches::kMemlogModeMinimal));
INSTANTIATE_TEST_CASE_P(AllProcesses,
                        MemlogBrowserTest,
                        ::testing::Values(switches::kMemlogModeAll));
INSTANTIATE_TEST_CASE_P(BrowserOnly,
                        MemlogBrowserTest,
                        ::testing::Values(switches::kMemlogModeBrowser));
INSTANTIATE_TEST_CASE_P(GpuOnly,
                        MemlogBrowserTest,
                        ::testing::Values(switches::kMemlogModeGpu));
INSTANTIATE_TEST_CASE_P(
    RendererSampling,
    MemlogBrowserTest,
    ::testing::Values(switches::kMemlogModeRendererSampling));

}  // namespace

#endif  // BUILDFLAG(USE_ALLOCATOR_SHIM)
