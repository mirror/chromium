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

// These functions are here to, respectively:
// 1. Check that functions are ordered
// 2. Delimit the start of .text
// 3. Delimit the end of .text
//
// (2) and (3) require a suitably constructed orderfile, with these
// functions at the beginning and end. (1) doesn't need to be in it.
//
// Also note that we have "load-bearing" CHECK()s below: they have side-effects.
// Without any code taking the address of a function, Identical Code Folding
// would merge these functions with other empty ones, defeating their purpose.
extern "C" {
void dummy_function_to_check_ordering() {}
void dummy_function_to_anchor_text() {}
void dummy_function_at_the_end_of_text() {}
}

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

// Used to compute the offset of any return address inside .text, as relative
// to this function.
const size_t kStartOfText =
    reinterpret_cast<size_t>(dummy_function_to_anchor_text);
const size_t kEndOfText =
    reinterpret_cast<size_t>(dummy_function_at_the_end_of_text);

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
    CHECK_LT(kStartOfText, kEndOfText);
    CHECK_LT(kEndOfText - kStartOfText, kMaxTextSizeInBytes);
    CHECK_LT(kStartOfText,
             reinterpret_cast<size_t>(&dummy_function_to_check_ordering));

    std::thread([this]() {
      int count = kDelayInSeconds * 2;
      for (int i = 0; i < count; ++i) {
        CollectResidency();
        usleep(5e5);
      }
      // As Dump() is called from the same thread, ensures that the
      // base functions called in the dumping code will not spuriously appear
      // in the profile.
      g_enabled_and_array.store(nullptr, std::memory_order_relaxed);
      Dump();
      DumpResidency();
    })
        .detach();
  }

 private:
  struct TimestampAndResidency {
    uint64_t timestamp_nanos;
    std::vector<unsigned char> residency;

    TimestampAndResidency(uint64_t timestamp_nanos,
                          std::vector<unsigned char> residency)
        : timestamp_nanos(timestamp_nanos), residency(residency) {}
  };
  std::unique_ptr<std::vector<TimestampAndResidency>> residency_data_;

  // Collects the current residency of the native library.
  //
  // The native library location is known from the anchor symbols, and mincore()
  // is used to tell whether pages are resident in memory.
  // This is not threadsafe.
  void CollectResidency() {
    // Initialization at first use, to avoid doing non-trivial work in a static
    // constructor.
    if (!residency_data_)
      residency_data_ = std::make_unique<std::vector<TimestampAndResidency>>();

    // Not using base::TimeTicks() to not call too many base:: symbol that would
    // pollute the reached symbols dumps.
    struct timespec ts;
    if (HANDLE_EINTR(clock_gettime(CLOCK_MONOTONIC, &ts))) {
      PLOG(ERROR) << "Cannot get the time.";
      return;
    }
    uint64_t now = ts.tv_sec * 1e9 + ts.tv_nsec;

    // Though executable code start has to be aligned on a page boundary, this
    // is not necessarily the case for .text, as .plt typically precedes it.
    // Align start and size to page boundaries.
    size_t start_of_text_page = kStartOfText & ~(kPageSize - 1);
    size_t text_size = kEndOfText - start_of_text_page;
    size_t text_size_in_pages =
        text_size / kPageSize + (text_size % kPageSize ? 1 : 0);
    auto data = std::vector<unsigned char>(text_size_in_pages);
    int err = HANDLE_EINTR(mincore(reinterpret_cast<void*>(start_of_text_page),
                                   text_size_in_pages * kPageSize, &data[0]));
    if (err) {
      PLOG(ERROR) << "mincore() failed";
      return;
    }

    residency_data_->emplace_back(now, std::move(data));
  }

  // Dumps the residency historical data to a file, and clears it.
  //
  // File format: <ns since Epoch> 0100011100111
  // Where each 0 or 1 represents a single page of .text.
  void DumpResidency() {
    auto path = base::FilePath(base::StringPrintf(
        "/data/local/tmp/chrome/residency-%d.txt", getpid()));
    auto file = base::File(
        path, base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
    if (!file.IsValid()) {
      LOG(ERROR) << "Cannot open the file";
      return;
    }

    for (const auto& data_point : *residency_data_) {
      auto timestamp =
          base::StringPrintf("%" PRIu64 " ", data_point.timestamp_nanos);
      file.WriteAtCurrentPos(timestamp.c_str(), timestamp.size());

      std::vector<char> dump;
      dump.reserve(data_point.residency.size() + 1);
      for (auto c : data_point.residency)
        dump.push_back(c ? '1' : '0');
      dump[dump.size() - 1] = '\n';
      file.WriteAtCurrentPos(&dump[0], dump.size());
    }
    residency_data_ = nullptr;
  }

  // Dumps the data to disk.
  static void Dump() {
    CHECK(!g_enabled_and_array.load(std::memory_order_relaxed));

    auto path = base::StringPrintf(
        "/data/local/tmp/chrome/cygprofile-instrumented-code-hitmap-%d.txt",
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
  static constexpr size_t kPageSize = 1 << 12;
};

// Static initializer on purpose. Will disable instrumentation after
// |kDelayInSeconds|.
DelayedDumper g_dump_later;

extern "C" {

// Since this function relies on the return address, if it's not inlined and
// __cyg_profile_func_enter() is called below, then the return address will
// be inside __cyg_profile_func_enter(). To prevent that, force inlining.
// We cannot use ALWAYS_INLINE from src/base/compiler_specific.h, as it doesn't
// always map to always_inline, for instance when NDEBUG is not defined.
__attribute__((__always_inline__)) void mcount() {
  // To avoid any risk of infinite recusion, this *must* *never* call any
  // instrumented function.
  auto* array = g_enabled_and_array.load(std::memory_order_relaxed);
  if (!array)
    return;

  size_t return_address = reinterpret_cast<size_t>(__builtin_return_address(0));
  // Not a CHECK() to avoid the possibility of calling an instrumented function.
  if (UNLIKELY(return_address < kStartOfText || return_address > kEndOfText)) {
    g_enabled_and_array.store(nullptr, std::memory_order_relaxed);
    LOG(FATAL) << "Return address out of bounds (wrong ordering?)";
  }

  size_t index = (return_address - kStartOfText) / sizeof(int);
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

}  // namespace
