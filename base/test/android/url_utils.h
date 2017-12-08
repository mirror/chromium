// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TEST_ANDROID_URL_UTILS_H_
#define BASE_TEST_ANDROID_URL_UTILS_H_

#include <jni.h>

#include "base/base_export.h"
#include "base/files/file_path.h"

namespace base {
namespace android {

BASE_EXPORT class UrlUtils {
 public:
  static FilePath GetIsolatedTestRoot();

 private:
  UrlUtils() = default;
  DISALLOW_COPY_AND_ASSIGN(UrlUtils);
};

}  // namespace android
}  // namespace base

#endif  // BASE_TEST_ANDROID_URL_UTILS_H_
