// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/cygprofile/lightweight_cygprofile.h"

#include <sys/types.h>
#include <unistd.h>
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
const size_t kSizeOfText = 288649995;  // 0xdeadbeef;
// Size of |symbols_array|, hence divide by sizeof(int).
constexpr size_t kSymbolArraySize = kSizeOfText / sizeof(int);

// Needs a patched orderfile, with __cyg_profile_func_exit as the first entry.
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
  symbols_array = new int[kSymbolArraySize];
  CHECK(symbols_array) << "Cannot allocate memory";
  // This is required, see comments in __cyg_profile_func_enter().
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

  // Two bad cases we don't check here, for performance reasons:
  // - this_fn < kStartOfText.
  // - this_fn - kStartOfText > kSymbolArraySize * sizeof(int)
  //
  // This could happen if:
  // - __cyg_profile_func_enter is not the first symbol:
  //     testing shows it's fine.
  // - kSizeOfText is inaccurate. We mandate that it must be accurate.
  //
  // More broadly, we cannot hit this code with |this_fn| outside of the range,
  // as code which is not compiled by us is not instrumented by the compiler.
  //
  // TODO(lizeb): Allocate the memory array using mmap(), and put guard pages
  // on both sides. This way we crash in case of out of bounds access without
  // paying for the check at each call.
  size_t offset = reinterpret_cast<size_t>(this_fn) -
                  reinterpret_cast<size_t>(kStartOfText);
  size_t index = offset / sizeof(int);

  // TODO(lizeb): Investigate adding a racy check of the array first, to reduce
  // false sharing.
  symbols_array[index] = 1;
}

void __cyg_profile_func_exit(void* this_fn, void* call_site) {}
}  // extern "C"
}  // namespace cygprofile
