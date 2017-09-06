// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/allocator/features.h"
#include "base/json/json_reader.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/browser/profiling_host/profiling_process_host.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
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

IN_PROC_BROWSER_TEST_P(MemlogBrowserTest, EndToEnd) {
  if (!GetParam()) {
    // Test that nothing has been started if the flag is not passed. Then early
    // exit.
    ASSERT_FALSE(profiling::ProfilingProcessHost::has_started());
    return;
  } else {
    ASSERT_TRUE(profiling::ProfilingProcessHost::has_started());
  }

  // Make some allocations in Browser. Then add a bunch of different sized
  // allocations to flush the buffer. The test should see an exact number of the
  // first size and some number of the second size.
  char* leak = nullptr;
  constexpr int kBrowserAllocSize = 103;
  constexpr int kNumBrowserAllocs = 500;
  constexpr int kBrowserAllocFlushFillSize = 107;
  constexpr int kNumBrowserAllocFills = 5000;
  for (int i = 0; i < kNumBrowserAllocs; ++i) {
    leak = new char[kBrowserAllocSize];
  }
  for (int i = 0; i < kNumBrowserAllocFills; ++i) {
    leak = new char[kBrowserAllocFlushFillSize];
  }

  // Make some allocations in Renderer.
  // TODO(ajwong): ^^^

  // Dump process the to json.
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  // Dump browser process to file.
  using profiling::ProfilingProcessHost;
  base::RunLoop run_loop1;
  run_loop1.RunUntilIdle();
  base::RunLoop run_loop;
  ProfilingProcessHost* pph = ProfilingProcessHost::GetInstance();
  base::FilePath dumpfile_path =
      temp_dir.GetPath().Append(FILE_PATH_LITERAL("dump.json.gz"));
  pph->RequestProcessDump(
      base::Process::Current().Pid(), dumpfile_path,
      base::BindOnce(&base::RunLoop::Quit, base::Unretained(&run_loop)));

  run_loop.Run();

  // Reparse the dump file.
  using base::File;
  File dumpfile(dumpfile_path, File::FLAG_OPEN | File::FLAG_READ);

  gzFile gz_file = gzdopen(dumpfile.TakePlatformFile(), "r");
  ASSERT_TRUE(gz_file) << "Cannot open compressed trace file";

  std::string dump_string;
  // Start with a 250k buffer because it's known that the dump will be large.
  // This avoids a bunch of unnecesasry resizing and copying of the string.
  // The 25-k number was arrived at by printing the uncompressed size of one
  // file during a single run on linux.
  constexpr size_t kInitialBufferSize = 250 * 1024;
  dump_string.reserve(kInitialBufferSize);

  char buf[4096];
  int bytes_read;
  while ((bytes_read = gzread(gz_file, buf, sizeof(buf))) == sizeof(buf)) {
    dump_string.append(buf, bytes_read);
  }
  ASSERT_GE(bytes_read, 0);
  EXPECT_GT(dump_string.size(), kInitialBufferSize / 2)
      << "Something has changed and initial buffer size is wastefully large "
         "now!";
  dump_string.append(buf, bytes_read);  // Grab last bytes.

  std::unique_ptr<base::Value> dump_json = base::JSONReader::Read(dump_string);

  // Verify allocation is found.
  // See chrome/profiling/json_exporter.cc for file format info.
  ASSERT_TRUE(dump_json);
  base::Value* events = dump_json->FindKey("traceEvents");
  ASSERT_GE(events->GetList().size(), 2UL);
  base::Value* dumps = &events->GetList()[1];
  LOG(ERROR) << dumps->type();

  base::Value* heaps_v2 = dumps->FindPath({"args", "dumps", "heaps_v2"});
  ASSERT_TRUE(heaps_v2);

  base::Value* sizes = heaps_v2->FindPath({"allocators", "malloc", "sizes"});
  ASSERT_TRUE(sizes);

  bool found_browser_alloc = false;
  bool found_browser_fill_alloc = false;
  const base::Value::ListStorage& sizes_list = sizes->GetList();
  size_t browser_alloc_index = 0;
  for (size_t i = 0; i < sizes_list.size(); i++) {
    if (sizes_list[i].GetInt() == kBrowserAllocSize) {
      found_browser_alloc = true;
    } else if (sizes_list[i].GetInt() == kBrowserAllocFlushFillSize) {
      found_browser_fill_alloc = true;
    }
  }

  ASSERT_TRUE(found_browser_fill_alloc)
      << "Fill alloc not found. Did the send buffer not flush?";
  ASSERT_TRUE(found_browser_alloc);

  base::Value* counts = heaps_v2->FindPath({"allocators", "malloc", "counts"});
  ASSERT_TRUE(counts);

  // This could be EXPECT_EQ, but that's not robust to others making allocs of
  // the give size.
  EXPECT_GE(counts->GetList()[browser_alloc_index].GetInt(), kNumBrowserAllocs);

  // TODO(ajwong): Verify renderer allocations are caught/exclueded as
  // necessary.
  // TODO(ajwong): Do same test for json endpoint.
}

// TODO(ajwong): Test what happens if profiling proccess crashes.

INSTANTIATE_TEST_CASE_P(NoMemlog,
                        MemlogBrowserTest,
                        ::testing::Values(static_cast<const char*>(nullptr)));
INSTANTIATE_TEST_CASE_P(BrowserOnly,
                        MemlogBrowserTest,
                        ::testing::Values(switches::kMemlogModeBrowser));
INSTANTIATE_TEST_CASE_P(AllProcesses,
                        MemlogBrowserTest,
                        ::testing::Values(switches::kMemlogModeAll));

}  // namespace

#endif  // BUILDFLAG(USE_ALLOCATOR_SHIM)
