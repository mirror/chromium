// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/test/views_pixel_test.h"

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "build/build_config.h"
#include "cc/test/pixel_comparator.h"
#include "cc/test/pixel_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"

#if defined(OS_WIN)
#include "base/win/windows_version.h"
#endif

namespace views {
namespace test {
namespace {

// Platform names. Note these are NOT "mac", "win", etc. because swarming will
// automatically filter those out when bundling files for testing bots.
#if defined(OS_MACOSX)
constexpr const char kPlatformName[] = "mac";
#elif defined(OS_CHROMEOS)
constexpr const char kPlatformName[] = "c";
#elif defined(OS_WIN)
constexpr const char kPlatformName[] = "win";
#else
constexpr const char kPlatformName[] = "linux";
#endif

base::FilePath GetDataPathRoot() {
  base::FilePath root_path;
  DCHECK(PathService::Get(base::DIR_SOURCE_ROOT, &root_path));
  return root_path.AppendASCII("ui")
      .AppendASCII("views")
      .AppendASCII("test")
      .AppendASCII("data");
}

}  // namespace

bool PixelTest(const SkBitmap& bitmap, const char* name, bool* used_fallback) {
  constexpr size_t kCommonIndex = 0;

  // Priority order of paths to try. If there is no common file, and no platform
  // file, start at the current platform index and try lower indexes until a
  // file is found. Then try higher indexes.
  const std::vector<std::string> paths = {"common", "linux", "c",
                                          "win",    "win10", "mac"};
  const base::FilePath data_path = GetDataPathRoot();

  auto index_of = [&paths](const char* platform) {
    return std::find(paths.begin(), paths.end(), platform) - paths.begin();
  };
  size_t start_index = index_of(kPlatformName);

#if defined(OS_WIN)
  if (base::win::GetVersion() >= base::win::VERSION_WIN10 &&
      base::PathExists(data_path.AppendASCII("win10").AppendASCII(name))) {
    start_index = index_of("win10");
  }
#endif

  size_t i = start_index;
  base::FilePath path = data_path.AppendASCII(paths[i]).AppendASCII(name);

  // If there's no platform-specific image. Try the common one next. Note this
  // does not update |i| for the |used_fallback| check below.
  if (!base::PathExists(path))
    path = data_path.AppendASCII(paths[kCommonIndex]).AppendASCII(name);

  // Hunt until a path is found.
  if (!base::PathExists(path)) {
    for (i = start_index - 1; i > kCommonIndex && !base::PathExists(path); --i)
      path = data_path.AppendASCII(paths[i]).AppendASCII(name);
  }

  // Search the other way.
  if (!base::PathExists(path)) {
    for (i = start_index + 1; i < paths.size() && !base::PathExists(path);
         ++i) {
      DLOG(INFO) << "trying " << path.value();
      path = data_path.AppendASCII(paths[i]).AppendASCII(name);
    }
  }

  if (i > paths.size()) {
    ADD_FAILURE() << "Couldn't find any reference image. Name: " << name;
    return false;
  }

  *used_fallback = i != start_index;
  if (*used_fallback)
    VLOG(0) << "Note: Using fallback reference image from " << path.value();

  return cc::MatchesPNGFile(bitmap, path,
                            cc::ExactPixelComparator(true /* discard alpha */));
}

void UpdateReferenceImage(const SkBitmap& bitmap, const char* name) {
  const base::FilePath path =
      GetDataPathRoot().AppendASCII(kPlatformName).AppendASCII(name);
  if (cc::WritePNGFile(bitmap, path, true /* discard transparency */))
    VLOG(0) << "Wrote to " << path.value();
  else
    PLOG(ERROR) << "FAILED to write to " << path.value();
}

}  // namespace test
}  // namespace views
