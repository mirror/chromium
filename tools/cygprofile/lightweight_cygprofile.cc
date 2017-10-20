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
#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/stringprintf.h"

extern "C" {
void dummy_function_to_anchor_text();

void __cyg_profile_func_enter(void* unused1, void* unused2)
    __attribute__((no_instrument_function));
void __cyg_profile_func_exit(void* unused1, void* unused2)
    __attribute__((no_instrument_function));
}

namespace {

constexpr size_t kTextSizeInBytes = 280000000;  // Must be an overestimate.
constexpr size_t kArraySize = kTextSizeInBytes / (4 * 32);

// Allocated in .bss. std::atomic<uint32_t> is guaranteed to behave as uint32_t
// with respect to size and initialization.
// Having the array in .bss means that instrumentation starts from the very
// first code executed within the binary.
std::atomic<uint32_t> g_return_offsets[kArraySize];

// Non-null iff collection is enabled. Also used as the pointer to the array
// to save a global load in the instrumentation function.
std::atomic<uint32_t>* g_enabled_and_array = g_return_offsets;

// This needs the orderfile to contain this function as the first entry.
// Used to compute th offset of any return address inside .text, as relative
// to this function.
const void* kStartOfText =
    reinterpret_cast<void*>(dummy_function_to_anchor_text);

// Sets a bit in a bitfield atomically. If the bit is already set, does nothing.
// |index| is the index of the bit to set.
void AtomicallySetBit(std::atomic<uint32_t>* bitfield, size_t index) {
  std::atomic<uint32_t>* element = bitfield + (index / 32);
  // First, a racy check. This saves a CAS if the bit is already set, and allows
  // the cache line to remain shared acoss CPUs in this case.
  uint32_t bitfield_element = element->load(std::memory_order_relaxed);
  uint32_t mask = (1 << (index % 32));
  if (bitfield_element & mask)
    return;

  uint32_t expected, desired;
  do {
    expected = element->load(std::memory_order_relaxed);
    desired = expected | mask;
  } while (element->compare_exchange_weak(expected, desired,
                                          std::memory_order_release));
}

extern "C" {
void dummy_function_to_anchor_text() {}
}

// Disables the logging and dumps the result after 30s.
class DelayedDumper {
 public:
  DelayedDumper() {
    // The linker usually keep the input file ordering for symbols.
    // dummy_function_to_anchor_text() should then be after AtomicallySetBit()
    // without ordering.
    // This check is thus intended to catch the lack of ordering.
    CHECK_LT(reinterpret_cast<size_t>(&dummy_function_to_anchor_text),
             reinterpret_cast<size_t>(&AtomicallySetBit));

    std::thread([]() {
      sleep(kDelayInS);
      // As Dump() is called from the same thread, ensures that the
      // base functions called in the dumping code will not spuriously appear
      // in the profile.
      g_enabled_and_array = nullptr;
      Dump();
    })
        .detach();
  }

 private:
  // Dumps the data to "/data/local/tmp/chrome/function-instrumentation-PID"
  static void Dump() {
    CHECK(!g_enabled_and_array);

    auto path = base::StringPrintf(
        "/data/local/tmp/chrome/function-instrumentation-%d.txt", getpid());
    auto file =
        base::File(base::FilePath(path),
                   base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
    if (!file.IsValid())
      return;

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

  static constexpr int kDelayInS = 30;
};

// Static initializer on purpose. Will disable instrumentation after 10s.
DelayedDumper g_dump_later;

extern "C" {

void __cyg_profile_func_enter(void* unused1, void* unused2) {
  auto* array = g_enabled_and_array;
  if (!array)
    return;

  void* return_address = __builtin_return_address(0);
  size_t offset = reinterpret_cast<size_t>(return_address) -
                  reinterpret_cast<size_t>(kStartOfText);
  size_t index = offset / sizeof(int);

  AtomicallySetBit(array, index);
}

void __cyg_profile_func_exit(void* unused1, void* unused2) {}

}  // extern "C"

}  // namespace
