// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains the implementation for TaskRunner::PostTaskAndReply.

#ifndef BASE_THREADING_POST_TASK_AND_REPLY_IMPL_H_
#define BASE_THREADING_POST_TASK_AND_REPLY_IMPL_H_

#include "base/base_export.h"
#include "base/callback.h"
#include "base/location.h"

namespace base {
namespace internal {

// Inherit from this in a class that implements PostTask to send a task to a
// custom execution context.
//
// If you're looking for a concrete implementation of PostTaskAndReply, you
// probably want base::TaskRunner.
//
// TODO(fdoray): Move this to the anonymous namespace of base/task_runner.cc.
class BASE_EXPORT PostTaskAndReplyImpl {
 public:
  virtual ~PostTaskAndReplyImpl() = default;

  // Invokes PostTask() with a callback that:
  // 1) Runs |task|.
  // 2) Posts |reply| back to the origin SequencedTaskRunner (i.e. the one
  //    returned by SequencedTaskRunnerHandle::Get() in the context from which
  //    this method is called).
  // If the callback passed to PostTask() is deleted before it runs, |task| is
  // deleted synchronously and a callback that owns |reply| is posted to the
  // origin SequencedTaskRunner.
  bool PostTaskAndReply(const Location& from_here,
                        OnceClosure task,
                        OnceClosure reply);

 private:
  virtual bool PostTask(const Location& from_here, OnceClosure task) = 0;
};

}  // namespace internal
}  // namespace base

#endif  // BASE_THREADING_POST_TASK_AND_REPLY_IMPL_H_
