// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/cygprofile/lightweight_cygprofile.h"

#include <sys/mman.h>
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
void dummy_function_to_anchor_text() {}

// The GCC compiler callbacks, called on every function invocation providing
// addresses of caller and callee codes.
void __restricted_cyg_profile_func_enter()
    __attribute__((no_instrument_function));
}  // extern "C"

namespace {
// The first one holds the array we use to store the reached return addreses.
// The second one is used by the instrumentation to both know whether it's
// enabled and where to store the result. Compared with using two variables
// (bool enabled and the array), this saves a global variable load in the
// instrumentation function.
int* g_return_addresses_array = nullptr;
int* g_enabled_and_array = nullptr;

// This must be at least the size of .text.
// TODO(lizeb): Should be set by the toolchain. Compile once, find out the size
// of .text, then patch the source and recompile (or patch the binary).
const size_t kSizeOfText = 79019574;
// Functions are 4-bytes aligned on ARM. The mininal size of a function is 2
// (single blx instruction in THUMB 2 mode). However the instrumentation call
// takes 4 bytes, bumping the miniman function size to 8 bytes. Hence we only
// need an array element per 8 bytes of .text.
const size_t kReturnAddressesArraySize = kSizeOfText / 8;
// Needs a patched orderfile, with __cyg_profile_func_enter as the first entry.
// Testing on Android ARM indicates that the symbol is truly the first one in
// .text, so taking its address gives the true start of the executable code
// section.
const void* kStartOfText =
    reinterpret_cast<void*>(&dummy_function_to_anchor_text);

void Enable() {
  CHECK(g_return_addresses_array);
  g_enabled_and_array = g_return_addresses_array;
  base::subtle::MemoryBarrier();
}

void Disable() {
  g_enabled_and_array = nullptr;
  base::subtle::MemoryBarrier();
}

void Initialize() {
  CHECK(!g_enabled_and_array) << "Already enabled!";
  if (g_return_addresses_array)
    return;
  constexpr size_t kPageSize = 1 << 12;
  constexpr size_t padding_size = 100 * kPageSize;
  size_t size_rounded_up = kReturnAddressesArraySize * sizeof(int);
  size_rounded_up += kPageSize - (size_rounded_up % kPageSize);
  void* array_with_padding =
      mmap(nullptr, size_rounded_up + padding_size, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  CHECK(array_with_padding) << "Unable to mmap()";
  int err = mprotect(static_cast<char*>(array_with_padding) + size_rounded_up,
                     padding_size, PROT_NONE);
  CHECK(!err) << "Cannot make the guard pages inacessible.";

  // Intentionally leaky.
  g_return_addresses_array = reinterpret_cast<int*>(array_with_padding);

  // Required, see comments in __cyg_profile_func_enter().
  base::subtle::MemoryBarrier();
}

void Dump(const base::FilePath& path) {
  CHECK(!g_enabled_and_array);
  auto file =
      base::File(path, base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
  if (!file.IsValid())
    return;
  auto data = std::vector<char>(kReturnAddressesArraySize + 1, '\0');
  for (size_t i = 0; i < kReturnAddressesArraySize; i++)
    data[i] = g_return_addresses_array[i] ? '1' : '0';
  data[kReturnAddressesArraySize] = '\n';
  file.WriteAtCurrentPos(&data[0],
                         static_cast<int>(kReturnAddressesArraySize + 1));
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
void __restricted_cyg_profile_func_enter() {
  // This is intentionally racy. At worst, we lose a few symbols early on, and
  // log a few too many entries at the end. Since the array is leaky, it will
  // not go away and cause issues.
  //
  // This only works if there is a memory barrier between the array allocation,
  // and enabling logging. Having this constraints allows us to not have any
  // synchronization here.
  if (!g_enabled_and_array)
    return;

  void* return_address = __builtin_return_address(0);
  size_t offset = reinterpret_cast<size_t>(return_address) -
                  reinterpret_cast<size_t>(kStartOfText);
  size_t index = offset / (2 * sizeof(int));

  // This is intentionally racy.
  //
  // Setting an entry of an array to 1 requires to move the corresponding
  // cache line to "exclusive" (assuming MESI for the cache). Since the only
  // transition we allow is 0 -> 1 for an array element, reading the outdated
  // value (since there is no synchronization) is not a correctness issue, but
  // saves a write and hence some cache traffic in the case of a frequently
  // executed function.
  if (!g_enabled_and_array[index])
    g_enabled_and_array[index] = 1;
}
}  // extern "C"
}  // namespace cygprofile
