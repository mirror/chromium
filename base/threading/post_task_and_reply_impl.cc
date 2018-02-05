// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/post_task_and_reply_impl.h"

#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"

namespace base {

namespace {

void DeleteReply(OnceClosure reply) {}

// Holds a reply callback and a target SequencedTaskRunner.
//
// Guarantees that the reply callback will either:
// - be scheduled for execution on the the target SequencedTaskRunner,
// - be moved to another ReplyHolder, or,
// - be scheduled for deletion on the target SequencedTaskRunner (if none of the
//   above has appened when the ReplyHolder is deleted).
class ReplyHolder {
 public:
  ReplyHolder(const Location& from_here,
              OnceClosure reply,
              scoped_refptr<SequencedTaskRunner> task_runner)
      : from_here_(from_here),
        reply_(std::move(reply)),
        task_runner_(std::move(task_runner)) {
    DCHECK(reply_);
    DCHECK(task_runner_);
  }
  ReplyHolder(ReplyHolder&&) = default;

  ~ReplyHolder() {
    // If |this| is deleted before |reply_| has been posted to |task_runner_|,
    // schedule deletion of |reply_| on |task_runner_|.
    if (reply_ && !task_runner_->RunsTasksInCurrentSequence())
      task_runner_->PostTask(from_here_,
                             BindOnce(&DeleteReply, std::move(reply_)));
  }

  // No move assignment operator because this class has const members.

  void Post() {
    DCHECK(task_runner_);
    DCHECK(reply_);
    task_runner_->PostTask(from_here_, std::move(reply_));
  }

 private:
  const Location from_here_;
  OnceClosure reply_;
  const scoped_refptr<SequencedTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(ReplyHolder);
};

void RunTaskAndPostReply(OnceClosure task, ReplyHolder reply) {
  std::move(task).Run();
  reply.Post();
}

}  // namespace

namespace internal {

bool PostTaskAndReplyImpl::PostTaskAndReply(const Location& from_here,
                                            OnceClosure task,
                                            OnceClosure reply) {
  DCHECK(task) << from_here.ToString();
  DCHECK(reply) << from_here.ToString();

  return PostTask(from_here,
                  BindOnce(&RunTaskAndPostReply, std::move(task),
                           ReplyHolder(from_here, std::move(reply),
                                       SequencedTaskRunnerHandle::Get())));
}

}  // namespace internal

}  // namespace base
