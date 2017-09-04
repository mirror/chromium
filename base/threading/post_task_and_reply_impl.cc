// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/post_task_and_reply_impl.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/debug/leak_annotations.h"
#include "base/logging.h"
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

// Similar to PostTaskAndReplyRelay, this is the relay for
// PostAsyncTaskAndReply.
// This is the implementation to post a |reply| to the original
// thread/sequence, when callback passed to |task| is called.
// Also, if the callback is destroyed before called, it also posts the |reply|
// to the original thread/sequence, then delete it there (without calling).
class PostAsyncTaskAndReplyRelay {
 public:
  PostAsyncTaskAndReplyRelay(const tracked_objects::Location& from_here,
                             OnceCallback<void(OnceClosure)> task,
                             OnceClosure reply)
      : from_here_(from_here),
        origin_task_runner_(SequencedTaskRunnerHandle::Get()),
        task_(std::move(task)),
        reply_(std::move(reply)) {}

  ~PostAsyncTaskAndReplyRelay() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  }

  void RunTask() {
    // Owned Holder instance, so if this completion callback is destroyed
    // without invocation, in Holder's dtor, |reply_| will be posted back
    // to the |original_task_runner_|, then deleted there.
    std::move(task_).Run(BindOnce(&Holder::Run, Owned(new Holder(this))));
  }

 private:
  // Tracks the lifetime of completion callback passed to |task_|.
  class Holder {
   public:
    explicit Holder(PostAsyncTaskAndReplyRelay* relay) : relay_(relay) {}

    ~Holder() { relay_->PostReply(); }

    // Marks |called_| true, so that in MaybeRunReplyAndSelfDestruct(),
    // |reply_| is called.
    // Note that, it is not necessary to PostReply() here. It is done in
    // the Holder's dtor, because this is "Owned" so soon deleted.
    void Run() { relay_->called_ = true; }

   private:
    PostAsyncTaskAndReplyRelay* const relay_;
    DISALLOW_COPY_AND_ASSIGN(Holder);
  };

  void PostReply() {
    // If PostTask() fails, intentionally leak this instance.
    origin_task_runner_->PostTask(
        from_here_,
        BindOnce(&PostAsyncTaskAndReplyRelay::MaybeRunReplyAndSelfDestruct,
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

  DISALLOW_COPY_AND_ASSIGN(PostAsyncTaskAndReplyRelay);
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

bool PostTaskAndReplyImpl::PostAsyncTaskAndReply(
    const tracked_objects::Location& from_here,
    OnceCallback<void(OnceClosure)> task,
    OnceClosure reply) {
  DCHECK(!task.is_null()) << from_here.ToString();
  DCHECK(!reply.is_null()) << from_here.ToString();

  auto relay = std::make_unique<PostAsyncTaskAndReplyRelay>(
      from_here, std::move(task), std::move(reply));

  // PostTaskAndReplyRelay self-destructs after executing |reply|. On the flip
  // side though, it is intentionally leaked if the |task| doesn't complete
  // before the origin sequence stops executing tasks. Annotate |relay| as leaky
  // to avoid having to suppress every callsite which happens to flakily trigger
  // this race.
  ANNOTATE_LEAKING_OBJECT_PTR(relay.get());

  if (!PostTask(from_here, BindOnce(&PostAsyncTaskAndReplyRelay::RunTask,
                                    Unretained(relay.get())))) {
    return false;
  }

  // The ownership is passed to the PostTask().
  relay.release();
  return true;
}

}  // namespace internal

}  // namespace base
