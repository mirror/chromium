// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "base/bind.h"
#include "base/run_loop.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "services/resource_coordinator/public/cpp/memory_instrumentation/memory_instrumentation.h"
#include "testing/gmock/include/gmock/gmock.h"

using testing::Le;
using testing::Ge;
using testing::AllOf;
using memory_instrumentation::mojom::GlobalMemoryDumpPtr;
using memory_instrumentation::mojom::GlobalMemoryDump;
using memory_instrumentation::mojom::ProcessMemoryDumpPtr;
using memory_instrumentation::mojom::ProcessMemoryDump;
using memory_instrumentation::mojom::ProcessType;

namespace content {

class MemoryInstrumentationTest : public ContentBrowserTest {
 protected:
  void Navigate(Shell* shell) {
    NavigateToURL(shell, GetTestUrl("", "title.html"));
  }
};

uint64_t GetBrowserPrivateFootprintKb(const GlobalMemoryDump& global_dump) {
  ProcessMemoryDump* browser_dump = nullptr;
  for (const ProcessMemoryDumpPtr& dump : global_dump.process_dumps) {
    if (dump->process_type == ProcessType::BROWSER) {
      EXPECT_FALSE(browser_dump);
      browser_dump = dump.get();
    }
  }
  EXPECT_TRUE(browser_dump);
  return browser_dump->os_dump->private_footprint_kb;
}

GlobalMemoryDumpPtr DoGlobalDump() {
  GlobalMemoryDumpPtr result = nullptr;
  base::RunLoop run_loop;
  memory_instrumentation::MemoryInstrumentation::GetInstance()
      ->RequestGlobalDump(base::Bind(
          [](base::Closure quit_closure, GlobalMemoryDumpPtr* out_result,
             bool success, GlobalMemoryDumpPtr result) {
            EXPECT_TRUE(success);
            *out_result = std::move(result);
            quit_closure.Run();
          },
          run_loop.QuitClosure(), &result));
  run_loop.Run();
  return result;
}

IN_PROC_BROWSER_TEST_F(MemoryInstrumentationTest, PrivateFootprint) {
  Navigate(shell());

  const int64_t size = 10 * 1024 * 1024;
  const int64_t size_kb = size / 1024;
  const int64_t one_mb_in_kb = 1024;

  GlobalMemoryDumpPtr before_ptr = DoGlobalDump();

  std::unique_ptr<char[]> buffer = base::MakeUnique<char[]>(size);
  memset(buffer.get(), 1, size);
  auto* x = static_cast<volatile char*>(buffer.get());
  EXPECT_EQ(x[0] + x[size - 1], 2);

  GlobalMemoryDumpPtr during_ptr = DoGlobalDump();

  buffer.reset();

  GlobalMemoryDumpPtr after_ptr = DoGlobalDump();

  int64_t before_kb = GetBrowserPrivateFootprintKb(*before_ptr);
  int64_t during_kb = GetBrowserPrivateFootprintKb(*during_ptr);
  int64_t after_kb = GetBrowserPrivateFootprintKb(*after_ptr);
  EXPECT_THAT(after_kb - before_kb, AllOf(Ge(-size_kb / 10), Le(size_kb / 10)));
  EXPECT_THAT(during_kb - before_kb, AllOf(Ge(size_kb - one_mb_in_kb * 3),
                                           Le(size_kb + one_mb_in_kb * 3)));
  EXPECT_THAT(during_kb - after_kb, AllOf(Ge(size_kb - one_mb_in_kb * 3),
                                          Le(size_kb + one_mb_in_kb * 3)));
}

}  // namespace content
