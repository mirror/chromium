// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/post_task_and_reply_impl.h"

#include <utility>

#include "base/bind.h"
#include "base/debug/leak_annotations.h"
#include "base/hack.h"
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
  PostTaskAndReplyRelay(const Location& from_here,
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
    LOG(ERROR) << "Running original:" << from_here_.ToString();
    std::move(task_).Run();

    from_here2_.reset(new Location(
        from_here_.function_name(), from_here_.file_name(),
        from_here_.line_number() + 1000000, from_here_.program_counter()));
    LOG(ERROR) << "Running reply:" << from_here2_->ToString() << " on "
               << origin_task_runner_.get();
    bool is_load_index_entries = false;
    if (from_here2_->function_name() == std::string("LoadIndexEntries")) {
      HACK_WaitUntil(base::AboutToCheck);
      HACK_AdvanceState(base::AboutToCheck,
                        base::GoingToPostLoadIndexEntriesReply);
      is_load_index_entries = true;
    }

    bool is_init = false;
    if (from_here2_->function_name() == std::string("Init")) {
      HACK_WaitUntil(base::FlushedWorkerPool);
      HACK_AdvanceState(base::FlushedWorkerPool,
                        base::GoingToPostInit);
      is_init = true;
    }
    origin_task_runner_->PostTask(
        *from_here2_, BindOnce(&PostTaskAndReplyRelay::RunReplyAndSelfDestruct,
                               base::Unretained(this)));
    // |this| has been deleted.
    if (is_load_index_entries) {
      HACK_AdvanceState(base::GoingToPostLoadIndexEntriesReply,
                        base::PostedLoadIndexEntriesReply);
    }

    if (is_init) {
      HACK_AdvanceState(base::GoingToPostInit,
                        base::PostedInit);
    }

    //LOG(ERROR) << "  was posted";
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
  const Location from_here_;
  std::unique_ptr<Location> from_here2_;
  const scoped_refptr<SequencedTaskRunner> origin_task_runner_;
  OnceClosure reply_;
  OnceClosure task_;
};

}  // namespace

namespace internal {

bool PostTaskAndReplyImpl::PostTaskAndReply(const Location& from_here,
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

}  // namespace internal

}  // namespace base
