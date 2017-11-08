// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <sys/mman.h>
#include <unistd.h>

#include <atomic>
#include <cstdio>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "base/files/file.h"
#include "base/format_macros.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/posix/eintr_wrapper.h"
#include "base/strings/stringprintf.h"
#include "tools/cygprofile/anchor_functions.h"

namespace cygprofile {

namespace {

constexpr size_t kMaxTextSizeInBytes = 280000000;  // Must be an overestimate.
constexpr size_t kArraySize = kMaxTextSizeInBytes / (4 * 32);

// Allocated in .bss. std::atomic<uint32_t> is guaranteed to behave as uint32_t
// with respect to size and initialization.
// Having the array in .bss means that instrumentation starts from the very
// first code executed within the binary.
std::atomic<uint32_t> g_return_offsets[kArraySize];

// Non-null iff collection is enabled. Also used as the pointer to the array
// to save a global load in the instrumentation function.
std::atomic<std::atomic<uint32_t>*> g_enabled_and_array = {g_return_offsets};


// Disables the logging and dumps the result after |kDelayInSeconds|.
class DelayedDumper {
 public:
  DelayedDumper() {
    CHECK_LT(kEndOfText - kStartOfText, kMaxTextSizeInBytes);
    CheckOrderingSanity();

    std::thread([this]() {
      sleep(kDelayInSeconds);
      // As Dump() is called from the same thread, ensures that the
      // base functions called in the dumping code will not spuriously appear
      // in the profile.
      g_enabled_and_array.store(nullptr, std::memory_order_relaxed);
      Dump();
    })
        .detach();
  }

 private:
  // Dumps the data to disk.
  static void Dump() {
    CHECK(!g_enabled_and_array.load(std::memory_order_relaxed));

    auto path = base::StringPrintf(
        "/data/local/tmp/chrome/cyglog/"
        "cygprofile-instrumented-code-hitmap-%d.txt",
        getpid());
    auto file =
        base::File(base::FilePath(path),
                   base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
    if (!file.IsValid()) {
      PLOG(ERROR) << "Could not open " << path;
      return;
    }

    auto data = std::vector<char>(kArraySize * 32 + 1, '\0');
    for (size_t i = 0; i < kArraySize; i++) {
      uint32_t value = g_return_offsets[i].load(std::memory_order_relaxed);
      for (int bit = 0; bit < 32; bit++) {
        data[32 * i + bit] = value & (1 << bit) ? '1' : '0';
      }
    }
    data[32 * kArraySize] = '\n';
    file.WriteAtCurrentPos(&data[0], static_cast<int>(data.size()));
  }

  static constexpr int kDelayInSeconds = 30;
};

// Static initializer on purpose. Will disable instrumentation after
// |kDelayInSeconds|.
DelayedDumper g_dump_later;

}  // namespace
}  // namespace cygprofile

extern "C" {

// Since this function relies on the return address, if it's not inlined and
// __cyg_profile_func_enter() is called below, then the return address will
// be inside __cyg_profile_func_enter(). To prevent that, force inlining.
// We cannot use ALWAYS_INLINE from src/base/compiler_specific.h, as it doesn't
// always map to always_inline, for instance when NDEBUG is not defined.
__attribute__((__always_inline__)) void mcount() {
  // To avoid any risk of infinite recusion, this *must* *never* call any
  // instrumented function.
  auto* array = cygprofile::g_enabled_and_array.load(std::memory_order_relaxed);
  if (!array)
    return;

  size_t return_address = reinterpret_cast<size_t>(__builtin_return_address(0));
  // Not a CHECK() to avoid the possibility of calling an instrumented function.
  if (UNLIKELY(return_address < cygprofile::kStartOfText ||
               return_address > cygprofile::kEndOfText)) {
    cygprofile::g_enabled_and_array.store(nullptr, std::memory_order_relaxed);
    LOG(FATAL) << "Return address out of bounds (wrong ordering?)";
  }

  size_t index = (return_address - cygprofile::kStartOfText) / sizeof(int);
  // Atomically set the corresponding bit in the array.
  std::atomic<uint32_t>* element = array + (index / 32);
  // First, a racy check. This saves a CAS if the bit is already set, and allows
  // the cache line to remain shared acoss CPUs in this case.
  uint32_t value = element->load(std::memory_order_relaxed);
  uint32_t mask = (1 << (index % 32));
  if (value & mask)
    return;

  // On CAS and memory ordering:
  // - std::atomic::compare_exchange_weak() updates |value| (non-const
  //   reference)
  // - We do not need any barrier because no other memory location is
  //   read/modified in this thread, hence the relaxed memory ordering.
  uint32_t desired;
  do {
    desired = value | mask;
  } while (element->compare_exchange_weak(value, desired,
                                          std::memory_order_relaxed));
}

void __cyg_profile_func_enter(void* unused1, void* unused2) {
  // Requires __always_inline__ on mcount(), see above.
  mcount();
}

void __cyg_profile_func_exit(void* unused1, void* unused2) {}

}  // extern "C"
