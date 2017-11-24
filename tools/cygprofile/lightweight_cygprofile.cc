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

namespace cygprofile {

namespace {

constexpr size_t kMaxTextSizeInBytes = 0x02b46fe4;  // Must be an overestimate.
constexpr size_t kArraySize = kMaxTextSizeInBytes / (4 * 32);

// Allocated in .bss. std::atomic<uint32_t> is guaranteed to behave as uint32_t
// with respect to size and initialization.
// Having the array in .bss means that instrumentation starts from the very
// first code executed within the binary, as well as making it possible to swap
// arrays cheaply to change instrumentation strategies.
std::atomic<uint32_t> g_startup_return_offsets[kArraySize];
std::atomic<uint32_t> g_postload_return_offsets[kArraySize];

// Non-null iff collection is enabled. Also used as the pointer to the array
// to save a global load in the instrumentation function.
std::atomic<std::atomic<uint32_t>*> g_enabled_and_array = {g_startup_return_offsets};

// This needs the orderfile to contain this function as the first entry.
// Used to compute the offset of any return address inside .text, as relative
// to this function.
const size_t kStartOfText =
    reinterpret_cast<size_t>(dummy_function_to_anchor_text);

void ExtractReturnOffsets(std::atomic<uint32_t>* offsets,
                          std::vector<int8_t>* extracted_data) {
  *extracted_data = std::vector<int8_t>(kArraySize * 32 + 1, '\0');
  for (size_t i = 0; i < kArraySize; i++) {
    uint32_t value = offsets[i].load(std::memory_order_relaxed);
    for (int bit = 0; bit < 32; bit++) {
      (*extracted_data)[32 * i + bit] = value & (1 << bit) ? '1' : '0';
    }
  }
}

void DumpInstrumentationData() {
  // The linker usually keeps the input file ordering for symbols.
  // dummy_function_to_anchor_text() should then be after
  // dummy_function_to_check_ordering() without ordering.
  // This check is thus intended to catch the lack of ordering.
  CHECK_LT(kStartOfText,
           reinterpret_cast<size_t>(&dummy_function_to_check_ordering));

  CHECK(!g_enabled_and_array.load(std::memory_order_relaxed));

  std::vector<int8_t> data;
  ExtractReturnOffsets(g_startup_return_offsets, &data);
  DumpInstrumentationArray("startup", getpid(), data);
  ExtractReturnOffsets(g_postload_return_offsets, &data);
  DumpInstrumentationArray("postload", getpid(), data);
}

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

void Checkpoint(const std::string& option) {
  auto* current_value = g_enabled_and_array.load(std::memory_order_relaxed);

  if (!option.empty()) {
    CHECK_EQ(option, "postload") << "Only valid checkpoint option is postload";

    // Advance to postload if we aren't already there, otherwise do nothing.
    if (current_value != g_postload_return_offsets) {
      g_enabled_and_array.store(g_postload_return_offsets,
                                std::memory_order_relaxed);
    }
    return;
  }

  // If no option, advance through startup -> postload -> null sequence.
  if (current_value != g_postload_return_offsets) {
    g_enabled_and_array.store(g_postload_return_offsets,
                              std::memory_order_relaxed);
  } else if (current_value != nullptr) {
    g_enabled_and_array.store(nullptr, std::memory_order_relaxed);
  }
}

void DumpCheckpoints() {
  g_enabled_and_array.store(nullptr, std::memory_order_relaxed);
  DumpInstrumentationData();
}

void ExtractInstrumentationData(std::vector<int8_t>* startup_offsets,
                                std::vector<int8_t>* postload_offsets) {
  g_enabled_and_array.store(nullptr, std::memory_order_relaxed);
  ExtractReturnOffsets(g_startup_return_offsets, startup_offsets);
  ExtractReturnOffsets(g_postload_return_offsets, postload_offsets);
}

void DumpInstrumentationArray(const std::string& specifier,
                              int pid,
                              const std::vector<int8_t>& offset_array) {
  auto dir = base::FilePath("/data/data/com.google.android.apps.chrome");
  if (!base::PathExists(dir)) {
    PLOG(WARNING) << "Could not find " << dir
                  << ", it must be created manually. Trying to continue";
  }
  base::FilePath path = dir.Append(base::StringPrintf(
      "cygprofile-instrumented-code-hitmap-%s-%d.txt", specifier.c_str(), pid));

  auto file =
      base::File(path, base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
  if (!file.IsValid()) {
    PLOG(ERROR) << "Could not open " << path;
    return;
  }
  // static_cast<char*> as a char is neither an int8_t nor a uint8_t.
  file.WriteAtCurrentPos(reinterpret_cast<const char*>(&offset_array[0]),
                         static_cast<int>(offset_array.size()));
  file.WriteAtCurrentPos("\n", 1);
  PLOG(INFO) << "Wrote dump information to " << path;
}

}  // namespace cygprofile
