// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/post_task_and_reply_impl.h"

#include <utility>

#include "base/bind.h"
#include "base/debug/leak_annotations.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/sequence_checker.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"

namespace base {

namespace {

// This relay class remembers the sequence that it was created on, and ensures
// that both the |task| and |reply| Closures are deleted on this same sequence.
// Also, |task| is guaranteed to be deleted before |reply| is run or deleted.
//
// If RunReplyAndSelfDestruct() doesn't run because the originating execution
// context is no longer available, then the |task| and |reply| Closures are
// leaked. Leaking is considered preferable to having a thread-safetey
// violations caused by invoking the Closure destructor on the wrong sequence.
class PostTaskAndReplyRelay {
 public:
  PostTaskAndReplyRelay(const tracked_objects::Location& from_here,
                        OnceClosure task,
                        OnceClosure reply)
      : sequence_checker_(),
        from_here_(from_here),
        origin_task_runner_(SequencedTaskRunnerHandle::Get()),
        reply_(std::move(reply)),
        task_(std::move(task)) {}

  ~PostTaskAndReplyRelay() {
    DCHECK(sequence_checker_.CalledOnValidSequence());
  }

  void RunTaskAndPostReply() {
    std::move(task_).Run();
    origin_task_runner_->PostTask(
        from_here_, BindOnce(&PostTaskAndReplyRelay::RunReplyAndSelfDestruct,
                             base::Unretained(this)));
  }

 private:
  void RunReplyAndSelfDestruct() {
    DCHECK(sequence_checker_.CalledOnValidSequence());

    // Ensure |task_| has already been released before |reply_| to ensure that
    // no one accidentally depends on |task_| keeping one of its arguments alive
    // while |reply_| is executing.
    DCHECK(!task_);

    std::move(reply_).Run();

    // Cue mission impossible theme.
    delete this;
  }

  const SequenceChecker sequence_checker_;
  const tracked_objects::Location from_here_;
  const scoped_refptr<SequencedTaskRunner> origin_task_runner_;
  OnceClosure reply_;
  OnceClosure task_;
};

class PostTaskAndReplyAsyncRelay {
 public:
  PostTaskAndReplyAsyncRelay(const tracked_objects::Location& from_here,
                             OnceCallback<void(OnceClosure)> task,
                             OnceClosure reply)
      : from_here_(from_here),
        origin_task_runner_(SequencedTaskRunnerHandle::Get()),
        task_(std::move(task)),
        reply_(std::move(reply)) {}

  ~PostTaskAndReplyAsyncRelay() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  }

  void RunTask() {
    std::move(task_).Run(BindOnce(&Holder::Run, Owned(new Holder(this))));
  }

 private:
  // Tracks the lifetime of completion callback passed to |task_|.
  class Holder {
   public:
    explicit Holder(PostTaskAndReplyAsyncRelay* relay) : relay_(relay) {}

    ~Holder() { relay_->PostReply(); }

    void Run() { relay_->called_ = true; }

   private:
    PostTaskAndReplyAsyncRelay* const relay_;
    DISALLOW_COPY_AND_ASSIGN(Holder);
  };

  void PostReply() {
    origin_task_runner_->PostTask(
        from_here_,
        BindOnce(&PostTaskAndReplyAsyncRelay::MaybeRunReplyAndSelfDestruct,
                 base::Unretained(this)));
  }

  void MaybeRunReplyAndSelfDestruct() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    // Ensure |task_| has already been released before |reply_| to ensure that
    // no one accidentally depends on |task_| keeping one of its arguments alive
    // while |reply_| is executing.
    DCHECK(!task_);
    if (called_)
      std::move(reply_).Run();

    delete this;
  }

  SEQUENCE_CHECKER(sequence_checker_);
  const tracked_objects::Location from_here_;
  const scoped_refptr<SequencedTaskRunner> origin_task_runner_;
  OnceCallback<void(OnceClosure)> task_;
  OnceClosure reply_;
  bool called_ = false;

  DISALLOW_COPY_AND_ASSIGN(PostTaskAndReplyAsyncRelay);
};

}  // namespace

namespace internal {

bool PostTaskAndReplyImpl::PostTaskAndReply(
    const tracked_objects::Location& from_here,
    OnceClosure task,
    OnceClosure reply) {
  DCHECK(!task.is_null()) << from_here.ToString();
  DCHECK(!reply.is_null()) << from_here.ToString();
  PostTaskAndReplyRelay* relay =
      new PostTaskAndReplyRelay(from_here, std::move(task), std::move(reply));
  // PostTaskAndReplyRelay self-destructs after executing |reply|. On the flip
  // side though, it is intentionally leaked if the |task| doesn't complete
  // before the origin sequence stops executing tasks. Annotate |relay| as leaky
  // to avoid having to suppress every callsite which happens to flakily trigger
  // this race.
  ANNOTATE_LEAKING_OBJECT_PTR(relay);
  if (!PostTask(from_here, BindOnce(&PostTaskAndReplyRelay::RunTaskAndPostReply,
                                    Unretained(relay)))) {
    delete relay;
    return false;
  }

  return true;
}

bool PostTaskAndReplyImpl::PostTaskAndReplyAsync(
    const tracked_objects::Location& from_here,
    OnceCallback<void(OnceClosure)> task,
    OnceClosure reply) {
  DCHECK(!task.is_null()) << from_here.ToString();
  DCHECK(!reply.is_null()) << from_here.ToString();

  auto relay = MakeUnique<PostTaskAndReplyAsyncRelay>(
      from_here, std::move(task), std::move(reply));
  ANNOTATE_LEAKING_OBJECT_PTR(relay.get());
  if (!PostTask(from_here, BindOnce(&PostTaskAndReplyAsyncRelay::RunTask,
                                    Unretained(relay.get())))) {
    return false;
  }

  // The ownership is passed to the PostTask().
  relay.release();
  return true;
}

}  // namespace internal

}  // namespace base
