// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_TEST_CHILD_PROCESS_FOR_TESTING_H_
#define CONTENT_TEST_CHILD_PROCESS_FOR_TESTING_H_

#include "base/macros.h"
#include "content/child/child_process.h"

namespace content {

// Use this instead of ChildProcess in unit tests.
//
// Cleans up the TaskScheduler in its destructor (in production, the
// TaskScheduler is leaked).
class ChildProcessForTesting : public ChildProcess {
 public:
  ChildProcessForTesting();
  ~ChildProcessForTesting() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ChildProcessForTesting);
};

}  // namespace content

#endif  // CONTENT_TEST_CHILD_PROCESS_FOR_TESTING_H_
