// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/library_loader/library_prefetcher.h"

#include <stddef.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "base/android/library_loader/anchor_functions.h"
#include "base/bits.h"
#include "base/files/file.h"
#include "base/format_macros.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/posix/eintr_wrapper.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "build/build_config.h"

#if BUILDFLAG(SUPPORTS_CODE_ORDERING)

namespace base {
namespace android {

namespace {

// Android defines the background priority to this value since at least 2009
// (see Process.java).
constexpr int kBackgroundPriority = 10;
// Valid for all Android architectures.
constexpr size_t kPageSize = 4096;

// Reads a byte per page between |start| and |end| to force it into the page
// cache.
// Heap allocations, syscalls and library functions are not allowed in this
// function.
// Returns true for success.
#if defined(ADDRESS_SANITIZER)
// Disable AddressSanitizer instrumentation for this function. It is touching
// memory that hasn't been allocated by the app, though the addresses are
// valid. Furthermore, this takes place in a child process. See crbug.com/653372
// for the context.
__attribute__((no_sanitize_address))
#endif
bool Prefetch(size_t start, size_t end) {
  unsigned char* start_ptr = reinterpret_cast<unsigned char*>(start);
  unsigned char* end_ptr = reinterpret_cast<unsigned char*>(end);
  unsigned char dummy = 0;
  for (unsigned char* ptr = start_ptr; ptr < end_ptr; ptr += kPageSize) {
    // Volatile is required to prevent the compiler from eliminating this
    // loop.
    dummy ^= *static_cast<volatile unsigned char*>(ptr);
  }
  return true;
}

// Populates the per-page residency between |start| and |end| in |residency|. If
// successful, |residency| has the size of |end| - |start| in pages.
// Returns true for success.
bool Mincore(size_t start, size_t end, std::vector<unsigned char>* residency) {
  if (start % kPageSize || end % kPageSize)
    return false;
  size_t size = end - start;
  size_t size_in_pages = size / kPageSize;
  if (residency->size() != size_in_pages)
    residency->resize(size_in_pages);
  int err = HANDLE_EINTR(
      mincore(reinterpret_cast<void*>(start), size, &(*residency)[0]));
  PLOG_IF(ERROR, err) << "mincore() failed";
  return !err;
}

// Returns the start and end of .text, aligned to the lower and upper page
// boundaries, respectively.
std::pair<size_t, size_t> GetTextRange() {
  // |kStartOftext| may not be at the beginning of a page, since .plt can be
  // before it, yet in the same mapping for instance.
  size_t start_page = kStartOfText - kStartOfText % kPageSize;
  // Set the end to the page on which the beginning of the last symbol is. The
  // actual symbol may spill into the next page by a few bytes, but this is
  // outside of the executable code range anyway.
  size_t end_page = base::bits::Align(kEndOfText, kPageSize);
  return {start_page, end_page};
}

// Timestamp in ns since Unix Epoch, and residency, as returned by mincore().
struct TimestampAndResidency {
  uint64_t timestamp_nanos;
  std::vector<unsigned char> residency;

  TimestampAndResidency(uint64_t timestamp_nanos,
                        std::vector<unsigned char>&& residency)
      : timestamp_nanos(timestamp_nanos), residency(residency) {}
};

// Returns true for success.
bool CollectResidency(size_t start,
                      size_t end,
                      std::vector<TimestampAndResidency>* data) {
  // Not using base::TimeTicks() to not call too many base:: symbol that would
  // pollute the reached symbols dumps.
  struct timespec ts;
  if (HANDLE_EINTR(clock_gettime(CLOCK_MONOTONIC, &ts))) {
    PLOG(ERROR) << "Cannot get the time.";
    return false;
  }
  uint64_t now =
      static_cast<uint64_t>(ts.tv_sec) * 1000 * 1000 * 1000 + ts.tv_nsec;
  std::vector<unsigned char> residency;
  if (!Mincore(start, end, &residency))
    return false;

  data->emplace_back(now, std::move(residency));
  return true;
}

void DumpResidency(size_t start,
                   size_t end,
                   std::unique_ptr<std::vector<TimestampAndResidency>> data) {
  auto path = base::FilePath(
      base::StringPrintf("/data/local/tmp/chrome/residency-%d.txt", getpid()));
  auto file =
      base::File(path, base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
  if (!file.IsValid()) {
    PLOG(ERROR) << "Cannot open file to dump the residency data "
                << path.value();
    return;
  }

  // First line: start-end of text range.
  CHECK(IsOrderingSane());
  CHECK_LT(start, kStartOfText);
  CHECK_LT(kEndOfText, end);
  auto start_end = base::StringPrintf("%" PRIuS " %" PRIuS "\n",
                                      kStartOfText - start, kEndOfText - start);
  file.WriteAtCurrentPos(start_end.c_str(), start_end.size());

  for (const auto& data_point : *data) {
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
}
}  // namespace

// static
bool NativeLibraryPrefetcher::ForkAndPrefetchNativeLibrary() {
  // Avoid forking with cygprofile instrumentation because the latter performs
  // memory allocations.
#if defined(CYGPROFILE_INSTRUMENTATION)
  return false;
#endif

  if (!IsOrderingSane()) {
    LOG(WARNING) << "Incorrect code ordering";
    return false;
  }

  // Looking for ranges is done before the fork, to avoid syscalls and/or memory
  // allocations in the forked process. The child process inherits the lock
  // state of its parent thread. It cannot rely on being able to acquire any
  // lock (unless special care is taken in a pre-fork handler), including being
  // able to call malloc().
  const auto& range = GetTextRange();

  pid_t pid = fork();
  if (pid == 0) {
    setpriority(PRIO_PROCESS, 0, kBackgroundPriority);
    // _exit() doesn't call the atexit() handlers.
    _exit(Prefetch(range.first, range.second) ? 0 : 1);
  } else {
    if (pid < 0) {
      return false;
    }
    int status;
    const pid_t result = HANDLE_EINTR(waitpid(pid, &status, 0));
    if (result == pid) {
      if (WIFEXITED(status)) {
        return WEXITSTATUS(status) == 0;
      }
    }
    return false;
  }
}

// static
int NativeLibraryPrefetcher::PercentageOfResidentCode(size_t start,
                                                      size_t end) {
  size_t total_pages = 0;
  size_t resident_pages = 0;

  std::vector<unsigned char> residency;
  bool ok = Mincore(start, end, &residency);
  if (!ok)
    return -1;
  total_pages += residency.size();
  resident_pages += std::count_if(residency.begin(), residency.end(),
                                  [](unsigned char x) { return x & 1; });
  if (total_pages == 0)
    return -1;
  return static_cast<int>((100 * resident_pages) / total_pages);
}

// static
int NativeLibraryPrefetcher::PercentageOfResidentNativeLibraryCode() {
  if (!IsOrderingSane()) {
    LOG(WARNING) << "Incorrect code ordering";
    return -1;
  }
  const auto& range = GetTextRange();
  return PercentageOfResidentCode(range.first, range.second);
}

// static
void NativeLibraryPrefetcher::PeriodicallyCollectResidency() {
  CHECK_EQ(static_cast<long>(kPageSize), sysconf(_SC_PAGESIZE));

  const auto& range = GetTextRange();
  auto data = std::make_unique<std::vector<TimestampAndResidency>>();
  for (int i = 0; i < 60; ++i) {
    if (!CollectResidency(range.first, range.second, data.get()))
      return;
    usleep(2e5);
  }
  DumpResidency(range.first, range.second, std::move(data));
}

// static
void NativeLibraryPrefetcher::MadviseRandomText() {
  CHECK(IsOrderingSane());
  const auto& range = GetTextRange();
  size_t size = range.second - range.first;
  int err = madvise(reinterpret_cast<void*>(range.first), size, MADV_RANDOM);
  if (err) {
    PLOG(ERROR) << "madvise() failed";
  }
}

}  // namespace android
}  // namespace base
#endif  // BUILDFLAG(SUPPORTS_CODE_ORDERING)
