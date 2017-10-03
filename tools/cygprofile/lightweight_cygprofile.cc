// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/cygprofile/lightweight_cygprofile.h"

#include <sys/types.h>
#include <unistd.h>
#include <atomic>
#include <string>
#include <thread>

#include "base/atomicops.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"

namespace cygprofile {

extern "C" {
// The GCC compiler callbacks, called on every function invocation providing
// addresses of caller and callee codes.
void __cyg_profile_func_enter(void* this_fn, void* call_site)
    __attribute__((no_instrument_function));
void __cyg_profile_func_exit(void* this_fn, void* call_site)
    __attribute__((no_instrument_function));
}  // extern "C"

namespace {
bool g_enabled = false;        // Intentionally racy.
int* symbols_array = nullptr;  // Leaky.

// This must be the exact size of .text.
// TODO(lizeb): Should be set by the toolchain. Compile once, find out the size
// of .text, then patch the source and recompile (or patch the binary).
const size_t kSizeOfText = 269370115 + 3325;  // Expanded to fill a page;
const size_t kSymbolArraySize = kSizeOfText / 4;
std::atomic<int> g_out_of_range_count;
// Needs a patched orderfile, with __cyg_profile_func_enter as the first entry.
// Testing on Android ARM indicates that the symbol is truly the first one in
// .text, so taking its address gives the true start of the executable code
// section.
const void* kStartOfText = reinterpret_cast<void*>(&__cyg_profile_func_enter);

void Enable() {
  g_enabled = true;
  base::subtle::MemoryBarrier();
}

void Disable() {
  g_enabled = false;
  base::subtle::MemoryBarrier();
}

void Initialize() {
  CHECK(!g_enabled) << "Already enabled!";
  if (symbols_array)
    return;

  // Intentionally leaky.
  symbols_array = new int[kSymbolArraySize];
  CHECK(symbols_array) << "Unable to allocate memory";

  // Required, see comments in __cyg_profile_func_enter().
  base::subtle::MemoryBarrier();
}

void Dump(const base::FilePath& path) {
  CHECK(!g_enabled);
  auto file =
      base::File(path, base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
  if (!file.IsValid())
    return;
  auto data = std::vector<char>(kSymbolArraySize + 1, '\0');
  for (size_t i = 0; i < kSymbolArraySize; i++)
    data[i] = symbols_array[i] ? '1' : '0';
  data[kSymbolArraySize] = '\n';
  file.WriteAtCurrentPos(&data[0], static_cast<int>(kSymbolArraySize + 1));
}

}  // namespace

void LogAndDump(int duration_seconds) {
  LOG(WARNING) << "Here We Go!";
  Initialize();
  Enable();
  // Not using an existing thread or a Chrome thread to reduce interferences
  // from the instrumentation.
  std::thread([duration_seconds]() {
    sleep(duration_seconds);
    LOG(WARNING) << "Time to stop";
    Disable();
    sleep(1);
    LOG(WARNING) << "Number of out-of-range calls = " << g_out_of_range_count;
    LOG(WARNING) << "Time to dump";
    std::string path = base::StringPrintf("/sdcard/dump-%d.txt", getpid());
    Dump(base::FilePath(path));
    LOG(WARNING) << "Done";
  })
      .detach();
}

extern "C" {

void __cyg_profile_func_enter(void* this_fn, void* callee_unused) {
  // This is intentionally racy. At worst, we lose a few symbols early on, and
  // log a few too many entries at the end. Since the array is leaky, it will
  // not go away and cause issues.
  //
  // This only works if there is a memory barrier between the array allocation,
  // and enabling logging. Having this constraints allows us to not have any
  // synchronization here.
  if (!g_enabled)
    return;

  size_t offset = reinterpret_cast<size_t>(this_fn) -
                  reinterpret_cast<size_t>(kStartOfText);
  // TODO(lizeb): Annotate with "unlikely".
  if (this_fn < kStartOfText || offset > kSizeOfText) {
    g_enabled = false;
    g_out_of_range_count++;
    g_enabled = true;
    return;
  }
  size_t index = offset / sizeof(int);

  // This is intentionally racy.
  //
  // Setting an entry of an array to 1 requires to move the corresponding
  // cache line to "exclusive" (assuming MESI for the cache). Since the only
  // transition we allow is 0 -> 1 for an array element, reading the outdated
  // value (since there is no synchronization) is not a correctness issue, but
  // saves a write and hence some cache traffic in the case of a frequently
  // executed function.
  if (!symbols_array[index])
    symbols_array[index] = 1;
}

void __cyg_profile_func_exit(void* this_fn, void* call_site) {}
}  // extern "C"
}  // namespace cygprofile
