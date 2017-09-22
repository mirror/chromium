// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_TEST_TEST_UTILS_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_TEST_TEST_UTILS_H_

namespace blink {
namespace scheduler {

inline bool MessageLoopTaskCounter(size_t* count) {
  *count = *count + 1;
  return true;
}

inline void NopTask() {}

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_TEST_TEST_UTILS_H_
