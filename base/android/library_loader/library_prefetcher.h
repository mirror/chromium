// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_ANDROID_LIBRARY_LOADER_LIBRARY_PREFETCHER_H_
#define BASE_ANDROID_LIBRARY_LOADER_LIBRARY_PREFETCHER_H_

#include <jni.h>

#include <stdint.h>

#include "base/base_export.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"

namespace base {
namespace android {

// Forks and waits for a process prefetching the native library. This is done in
// a forked process for the following reasons:
// - Isolating the main process from mistakes in getting the address range, only
//   crashing the forked process will crash.
// - Not inflating the memory used by the main process uselessly, which could
//   increase its likelihood to be killed.
// The forked process has background priority and, since it is not declared to
// the Android runtime, can be killed at any time, which is not an issue here.
class BASE_EXPORT NativeLibraryPrefetcher {
 public:
  // Finds the executable code range, forks a low priority process pre-fetching
  // these ranges and wait()s for it.
  // Returns true for success.
  static bool ForkAndPrefetchNativeLibrary();

  // Returns the percentage of the native library code currently resident in
  // memory, or -1 in case of error.
  static int PercentageOfResidentNativeLibraryCode();

  // Collects residency for the native library executable multiple times, then
  // dumps it to disk.
  static void PeriodicallyCollectResidency();

  // Calls madvise(MADV_RANDOM) on the native library executable code range.
  static void MadviseRandomText();

 private:
  // Returns the percentage of the given address ranges currently resident in
  // memory, or -1 in case of error.
  static int PercentageOfResidentCode(size_t start, size_t end);

  FRIEND_TEST_ALL_PREFIXES(NativeLibraryPrefetcherTest,
                           TestPercentageOfResidentCode);

  DISALLOW_IMPLICIT_CONSTRUCTORS(NativeLibraryPrefetcher);
};

}  // namespace android
}  // namespace base

#endif  // BASE_ANDROID_LIBRARY_LOADER_LIBRARY_PREFETCHER_H_
