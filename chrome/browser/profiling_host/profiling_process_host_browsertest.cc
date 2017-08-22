// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/allocator/features.h"
#include "base/files/file_path.h"
#include "base/run_loop.h"
#include "chrome/browser/profiling_host/profiling_process_host.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/common/service_manager_connection.h"

#if BUILDFLAG(USE_ALLOCATOR_SHIM)
namespace profiling {

class ProfilingProcessHostBrowserTest : public InProcessBrowserTest {
 public:
  void ProcessDumpFinished(bool success) {
  }
 protected:
  void SetUp() override {
    InProcessBrowserTest::SetUp();
    ASSERT_TRUE(dir_.CreateUniqueTempDir());
    path_ = dir_.GetPath().Append("test_dump.json.gz");
  LOG(ERROR) << "asdf: " << path_.AsUTF8Unsafe();
  }

  base::ScopedTempDir dir_;
  base::FilePath path_;
};

IN_PROC_BROWSER_TEST_F(ProfilingProcessHostBrowserTest, BrowserProfilingDumpToFile) {
  // Start profiling the browser process.
  ProfilingProcessHost* host = ProfilingProcessHost::EnsureStarted(content::ServiceManagerConnection::GetForProcess(), ProfilingProcessHost::Mode::kBrowser);

  // Make some allocations.
  for (int i = 0; i < 10000; ++i) {
    free(malloc(1));
  }

  base::RunLoop run_loop;
  LOG(ERROR) << path_.AsUTF8Unsafe();
  ProfilingProcessHost::RequestProcessDumpCallback callback = base::Bind(&ProfilingProcessHostBrowserTest::ProcessDumpFinished, base::Unretained(this));
  host->RequestProcessDump(
      base::Process::Current().Pid(),
      path_, std::move(callback));
  run_loop.Run();

  base::File file(path_, base::File::FLAG_OPEN | base::File::FLAG_READ);
  ASSERT_TRUE(file.IsValid());
  base::File::Info info;
  ASSERT_TRUE(file.GetInfo(&info));
  LOG(ERROR) << info.size;
}

}  // namespace profiling
#endif  // BUILDFLAG(USE_ALLOCATOR_SHIM)
