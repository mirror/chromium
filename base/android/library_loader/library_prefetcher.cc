// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/library_loader/library_prefetcher.h"

#include <fcntl.h>
#include <linux/fadvise.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <utility>
#include <vector>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/format_macros.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/posix/eintr_wrapper.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"

namespace base {
namespace android {

namespace {

// Android defines the background priority to this value since at least 2009
// (see Process.java).
constexpr int kBackgroundPriority = 10;
// Valid for all the Android architectures.
constexpr size_t kPageSize = 4096;
constexpr size_t kPageMask = kPageSize - 1;
const char* kLibchromeSuffix = "libchrome.so";
// "base.apk" is a suffix because the library may be loaded directly from the
// APK.
const char* kSuffixesToMatch[] = {kLibchromeSuffix, "base.apk"};

// Used to speed up |CollectResidency()|, as parsing /proc/self/smaps can be
// slow.
base::LazyInstance<std::vector<NativeLibraryPrefetcher::AddressRange>>::Leaky
    g_ranges;

bool IsReadableAndPrivate(const base::debug::MappedMemoryRegion& region) {
  return region.permissions & base::debug::MappedMemoryRegion::READ &&
         region.permissions & base::debug::MappedMemoryRegion::PRIVATE;
}

bool PathMatchesSuffix(const std::string& path) {
  for (size_t i = 0; i < arraysize(kSuffixesToMatch); i++) {
    if (EndsWith(path, kSuffixesToMatch[i], CompareCase::SENSITIVE)) {
      return true;
    }
  }
  return false;
}

// For each range, reads a byte per page to force it into the page cache.
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
bool Prefetch(const std::vector<std::pair<uintptr_t, uintptr_t>>& ranges) {
  for (const auto& range : ranges) {
    // If start or end is not page-aligned, parsing went wrong. It is better to
    // exit with an error.
    if ((range.first & kPageMask) || (range.second & kPageMask)) {
      return false;  // CHECK() is not allowed here.
    }
    unsigned char* start_ptr = reinterpret_cast<unsigned char*>(range.first);
    unsigned char* end_ptr = reinterpret_cast<unsigned char*>(range.second);
    unsigned char dummy = 0;
    for (unsigned char* ptr = start_ptr; ptr < end_ptr; ptr += kPageSize) {
      // Volatile is required to prevent the compiler from eliminating this
      // loop.
      dummy ^= *static_cast<volatile unsigned char*>(ptr);
    }
  }
  return true;
}

// Calls fadvise(DONTNEED) on |filename|. Returns true for success.
bool FadviseDontNeedFile(const std::string& filename) {
  ScopedFD fd(open(filename.c_str(), O_RDONLY));
  if (!fd.is_valid()) {
    LOG(ERROR) << "Cannot open " << filename;
    return false;
  }

  struct stat stat_buf;
  int err = fstat(fd.get(), &stat_buf);
  if (err) {
    LOG(ERROR) << "stat() error.";
    return false;
  }

#if defined(ARCH_CPU_ARMEL)
  // We have to use the syscall directly as bionic doesn't export the wrapper,
  // even though the syscall is supported on all Android versions.
  // Arguments are flipped on ARM because 64 bit values have to start on an
  // even register number.
  // Also, offset is 0 here, and we "dangerously" assume that the native
  // library is <4GB.
  err = syscall(__NR_arm_fadvise64_64, fd.get(), POSIX_FADV_DONTNEED, 0, 0, 0,
                static_cast<unsigned int>(stat_buf.st_size));

  if (err) {
    LOG(ERROR) << "fadvise() error.";
    return false;
  }
  return true;
#else
  return false;
#endif  // defined(ARCH_CPU_ARMEL)
}

}  // namespace

// static
bool NativeLibraryPrefetcher::IsGoodToPrefetch(
    const base::debug::MappedMemoryRegion& region) {
  return PathMatchesSuffix(region.path) &&
         IsReadableAndPrivate(region);  // .text and .data mappings are private.
}

// static
void NativeLibraryPrefetcher::FilterLibchromeRangesOnlyIfPossible(
    const std::vector<base::debug::MappedMemoryRegion>& regions,
    std::vector<AddressRange>* ranges) {
  bool has_libchrome_region = false;
  for (const base::debug::MappedMemoryRegion& region : regions) {
    if (EndsWith(region.path, kLibchromeSuffix, CompareCase::SENSITIVE)) {
      has_libchrome_region = true;
      break;
    }
  }
  for (const base::debug::MappedMemoryRegion& region : regions) {
    if (has_libchrome_region &&
        !EndsWith(region.path, kLibchromeSuffix, CompareCase::SENSITIVE)) {
      continue;
    }
    ranges->push_back(std::make_pair(region.start, region.end));
  }
}

// static
bool NativeLibraryPrefetcher::FindRanges(std::vector<AddressRange>* ranges) {
  std::string proc_maps;
  if (!base::debug::ReadProcMaps(&proc_maps))
    return false;
  std::vector<base::debug::MappedMemoryRegion> regions;
  if (!base::debug::ParseProcMaps(proc_maps, &regions))
    return false;

  std::vector<base::debug::MappedMemoryRegion> regions_to_prefetch;
  for (const auto& region : regions) {
    if (IsGoodToPrefetch(region)) {
      regions_to_prefetch.push_back(region);
    }
  }

  FilterLibchromeRangesOnlyIfPossible(regions_to_prefetch, ranges);
  return true;
}

// static
bool NativeLibraryPrefetcher::ForkAndPrefetchNativeLibrary() {
  // Avoid forking with cygprofile instrumentation because the latter performs
  // memory allocations.
#if defined(CYGPROFILE_INSTRUMENTATION)
  return false;
#endif

  // Looking for ranges is done before the fork, to avoid syscalls and/or memory
  // allocations in the forked process. The child process inherits the lock
  // state of its parent thread. It cannot rely on being able to acquire any
  // lock (unless special care is taken in a pre-fork handler), including being
  // able to call malloc().
  std::vector<AddressRange> ranges;
  if (!FindRanges(&ranges))
    return false;

  pid_t pid = fork();
  if (pid == 0) {
    setpriority(PRIO_PROCESS, 0, kBackgroundPriority);
    // _exit() doesn't call the atexit() handlers.
    _exit(Prefetch(ranges) ? 0 : 1);
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
int NativeLibraryPrefetcher::PercentageOfResidentCode(
    const std::vector<AddressRange>& ranges) {
  size_t total_pages = 0;
  size_t resident_pages = 0;

  for (const auto& range : ranges) {
    if (range.first & kPageMask || range.second & kPageMask)
      return -1;
    size_t length = range.second - range.first;
    size_t pages = length / kPageSize;
    total_pages += pages;
    std::vector<unsigned char> is_page_resident(pages);
    int err = mincore(reinterpret_cast<void*>(range.first), length,
                      &is_page_resident[0]);
    DPCHECK(!err);
    if (err)
      return -1;
    resident_pages +=
        std::count_if(is_page_resident.begin(), is_page_resident.end(),
                      [](unsigned char x) { return x & 1; });
  }
  if (total_pages == 0)
    return -1;
  return static_cast<int>((100 * resident_pages) / total_pages);
}

// static
int NativeLibraryPrefetcher::PercentageOfResidentNativeLibraryCode() {
  if (!MaybeCollectRanges())
    return -1;
  return PercentageOfResidentCode(g_ranges.Get());
}

// static
bool NativeLibraryPrefetcher::MaybeCollectRanges() {
  if (g_ranges.Get().empty()) {
    if (!FindRanges(&g_ranges.Get())) {
      LOG(ERROR) << "Unable to find the ranges";
      return false;
    }
  }
  return true;
}

// static
bool NativeLibraryPrefetcher::CollectResidency(const base::FilePath& filename) {
  TRACE_EVENT_BEGIN1("startup", "NativeLibraryPrefetcher::CollectResidency",
                     "percentage", 0);

  if (!MaybeCollectRanges())
    return false;

  base::File file(filename,
                  base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
  if (!file.IsValid()) {
    LOG(ERROR) << "File is not valid " << filename;
    return false;
  }

  for (const auto& range : g_ranges.Get()) {
    if (range.first & kPageMask || range.second & kPageMask) {
      LOG(ERROR) << "Invalid range";
      return false;
    }
    size_t length = range.second - range.first;
    size_t pages = length / kPageSize;
    std::vector<unsigned char> is_page_resident(pages);
    int err = mincore(reinterpret_cast<void*>(range.first), length,
                      &is_page_resident[0]);
    if (err) {
      LOG(ERROR) << "mincore() failed, err = " << err;
      return false;
    }

    // File format:
    // [start address] 000111101111\n
    // Where each 0 or 1 is a single page, and reflects mincore()'s result.
    std::string start_address = base::StringPrintf("%" PRIuS " ", range.first);
    file.WriteAtCurrentPos(start_address.c_str(), start_address.size());

    std::string mincore_result(pages + 1, ' ');
    mincore_result[mincore_result.size() - 1] = '\n';
    for (size_t i = 0; i < pages; ++i)
      mincore_result[i] = is_page_resident[i] ? '1' : '0';
    file.WriteAtCurrentPos(mincore_result.c_str(), mincore_result.size());
  }
  int percentage = PercentageOfResidentNativeLibraryCode();
  TRACE_EVENT_END1("startup", "NativeLibraryPrefetcher::CollectResidency",
                   "percentage", percentage);
  return true;
}

// static
bool NativeLibraryPrefetcher::Purge(const std::string& path, int size_mb) {
  TRACE_EVENT0("startup", "NativeLibraryPrefetcher::Purge");
  const std::string library_path = path + "/libchrome.so";
  bool ok = FadviseDontNeedFile(library_path);

  if (!MaybeCollectRanges())
    return false;
  for (const auto& range : g_ranges.Get()) {
    size_t length = range.second - range.first;
    int err =
        madvise(reinterpret_cast<void*>(range.first), length, MADV_RANDOM);
    if (err) {
      LOG(ERROR) << "madvise() failed, err = " << err;
      return false;
    }
  }

  if (size_mb == 0)
    return true;

  // Take memory, outsmart the compiler.
  size_t sum = 0;
  const size_t size = size_mb * 1e6;
  {
    TRACE_EVENT0("startup", "NativeLibraryPrefetcher::Purge::Memset");
    void* data = malloc(size);
    memset(data, ok ? 1 : 0, size);
    for (size_t i = 0; i < size; ++i) {
      sum += reinterpret_cast<unsigned char*>(data)[i];
    }
    free(data);
  }
  return sum == size;
}

}  // namespace android
}  // namespace base
