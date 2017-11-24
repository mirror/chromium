// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <unistd.h>

#include <atomic>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "tools/cygprofile/lightweight_cygprofile.h"

extern "C" {
void dummy_function_to_check_ordering() {}
void dummy_function_to_anchor_text() {}
}

namespace {

constexpr size_t kMaxTextSizeInBytes = 0x02b458b4;  // Must be an overestimate.
constexpr size_t kArraySize = kMaxTextSizeInBytes / (4 * 32);

// Allocated in .bss. std::atomic<uint32_t> is guaranteed to behave as uint32_t
// with respect to size and initialization.
// Having the array in .bss means that instrumentation starts from the very
// first code executed within the binary, as well as making it possible to swap
// arrays cheaply to change instrumentation strategies.
std::atomic<uint32_t> g_startup_return_offsets[kArraySize];
std::atomic<uint32_t> g_post_loading_return_offsets[kArraySize];

// Non-null iff collection is enabled. Also used as the pointer to the array
// to save a global load in the instrumentation function.
std::atomic<std::atomic<uint32_t>*> g_enabled_and_array = {g_startup_return_offsets};

// This needs the orderfile to contain this function as the first entry.
// Used to compute the offset of any return address inside .text, as relative
// to this function.
const size_t kStartOfText =
    reinterpret_cast<size_t>(dummy_function_to_anchor_text);

// Disables the logging and dumps the result after |kDelayInSeconds|.
class DelayedDumper {
 public:
  DelayedDumper() {
    // The linker usually keeps the input file ordering for symbols.
    // dummy_function_to_anchor_text() should then be after
    // dummy_function_to_check_ordering() without ordering.
    // This check is thus intended to catch the lack of ordering.
    CHECK_LT(kStartOfText,
             reinterpret_cast<size_t>(&dummy_function_to_check_ordering));

    std::thread([]() {
      // TODO(mattcary): explicit dump signal rather than waiting. Particularly
      // with the array check below, we could race on the dump.
      sleep(kDelayInSeconds);
      // As Dump() is called from the same thread, ensures that the
      // base functions called in the dumping code will not spuriously appear
      // in the profile.

      // Delay dump until post_loading array has been used.
      // TODO(mattcary): confirm std::memory_order_relaxed usage.
      while (g_enabled_and_array.load(std::memory_order_relaxed) !=
             g_post_loading_return_offsets) {
        sleep(kDelayInSeconds);
      }

      g_enabled_and_array.store(nullptr, std::memory_order_relaxed);
      Dump();
    })
        .detach();
  }

 private:
  // Dumps the data to disk.
  static void Dump() {
    CHECK(!g_enabled_and_array.load(std::memory_order_relaxed));

    DumpArray("startup", g_startup_return_offsets);
    DumpArray("postload", g_post_loading_return_offsets);
  }

  static void DumpArray(const std::string& specifier,
                        std::atomic<uint32_t>* offset_array) {
    auto dir = base::FilePath("/data/local/tmp/chrome");
    if (!base::PathExists(dir)) {
      PLOG(WARNING) << "Could not find " << dir
                  << ", it must be created manually. Trying to continue";
    }
    base::FilePath path = dir.Append(
        base::StringPrintf("cygprofile-instrumented-code-hitmap-%s-%d.txt",
                           specifier.c_str(), getpid()));

    auto file = base::File(
        path, base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
    if (!file.IsValid()) {
      PLOG(ERROR) << "Could not open " << path;
      return;
    }

    auto data = std::vector<char>(kArraySize * 32 + 1, '\0');
    for (size_t i = 0; i < kArraySize; i++) {
      uint32_t value = offset_array[i].load(std::memory_order_relaxed);
      for (int bit = 0; bit < 32; bit++) {
        data[32 * i + bit] = value & (1 << bit) ? '1' : '0';
      }
    }
    data[32 * kArraySize] = '\n';
    file.WriteAtCurrentPos(&data[0], static_cast<int>(data.size()));
    PLOG(INFO) << "Wrote dump information to " << path;
  }

  static constexpr int kDelayInSeconds = 60;
};

// Static initializer on purpose. Will disable instrumentation after
// |kDelayInSeconds|.
DelayedDumper g_dump_later;

extern "C" {
void __cyg_profile_func_enter_bare() {
  // To avoid any risk of infinite recusion, this *must* *never* call any
  // instrumented function.
  auto* array = g_enabled_and_array.load(std::memory_order_relaxed);
  if (!array)
    return;

  size_t return_address = reinterpret_cast<size_t>(__builtin_return_address(0));
  size_t index = (return_address - kStartOfText) / sizeof(int);
  // Not a CHECK() to avoid the possibility of calling an instrumented function.
  if (UNLIKELY(return_address < kStartOfText || index / 32 > kArraySize)) {
    g_enabled_and_array.store(nullptr, std::memory_order_relaxed);
    LOG(FATAL) << "Return address out of bounds (wrong ordering, "
               << ".text larger than the max size?)";
  }

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

}  // extern "C"

}  // namespace

namespace cygprofile {

void OnDidStopLoading() {
  // Swap return offset array.
  g_enabled_and_array.store(g_post_loading_return_offsets,
                            std::memory_order_relaxed);
}

}  // namespace cygprofile
