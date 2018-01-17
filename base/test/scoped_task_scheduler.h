// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TEST_SCOPED_TASK_SCHEDULER_H_
#define BASE_TEST_SCOPED_TASK_SCHEDULER_H_

#include "base/macros.h"
#include "base/strings/string_piece.h"

namespace base {
namespace test {

// Generally, you should use ScopedTaskEnvironment instead of this class.
// This class provides a TaskScheduler for the convenience of base unit tests
// that create a MessageLoop.
class ScopedTaskScheduler final {
 public:
  ScopedTaskScheduler(StringPiece name);
  ~ScopedTaskScheduler();

 private:
  DISALLOW_COPY_AND_ASSIGN(ScopedTaskScheduler);
};

}  // namespace test
}  // namespace base

#endif  // BASE_TEST_SCOPED_TASK_SCHEDULER_H_
