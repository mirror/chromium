// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_IOS_SCOPED_CRITICAL_ACTION_H_
#define BASE_IOS_SCOPED_CRITICAL_ACTION_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"

namespace base {
namespace ios {

// This class attempts to allow the application to continue to run for a period
// of time after it transitions to the background. The construction of an
// instance of this class marks the beginning of a task that needs background
// running time when the application is moved to the background and the
// destruction marks the end of such a task.
//
// Note there is no guarantee that the task will continue to finish when the
// application is moved to the background.
//
// This class should be used at times where leaving a task unfinished might be
// detrimental to user experience. For example, it should be used to ensure that
// the application has enough time to save important data or at least attempt to
// save such data.
class ScopedCriticalAction {
 public:
  ScopedCriticalAction();
  ~ScopedCriticalAction();

 private:
  // Task identifier.
  int task_id_;

  DISALLOW_COPY_AND_ASSIGN(ScopedCriticalAction);
};

}  // namespace ios
}  // namespace base

#endif  // BASE_IOS_SCOPED_CRITICAL_ACTION_H_
