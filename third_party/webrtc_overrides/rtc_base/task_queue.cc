/*
 *  Copyright 2016 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "third_party/webrtc/rtc_base/task_queue.h"

#include "base/bind.h"
#include "base/lazy_instance.h"
#include "base/memory/ref_counted.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread.h"
#include "base/threading/thread_local.h"
#include "build/build_config.h"
#include "third_party/webrtc/rtc_base/refcount.h"
#include "third_party/webrtc/rtc_base/refcountedobject.h"

// Intentionally outside of the "namespace rtc { ... }" block, because
// here, scoped_refptr should *not* be resolved as rtc::scoped_refptr.
namespace {

void RunTask(std::unique_ptr<rtc::QueuedTask> task) {
  if (!task->Run())
    task.release();
}

class PostAndReplyTask : public rtc::QueuedTask {
 public:
  PostAndReplyTask(
      std::unique_ptr<rtc::QueuedTask> task,
      std::unique_ptr<rtc::QueuedTask> reply,
      const scoped_refptr<base::SingleThreadTaskRunner>& reply_task_runner)
      : task_(std::move(task)),
        reply_(std::move(reply)),
        reply_task_runner_(reply_task_runner) {}

  ~PostAndReplyTask() override {}

 private:
  bool Run() override {
    if (!task_->Run())
      task_.release();

    reply_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&RunTask, base::Passed(&reply_)));
    return true;
  }

  std::unique_ptr<rtc::QueuedTask> task_;
  std::unique_ptr<rtc::QueuedTask> reply_;
  scoped_refptr<base::SingleThreadTaskRunner> reply_task_runner_;
};

class PostAndReplyTask2 : public rtc::QueuedTask {
 public:
  PostAndReplyTask2(
      std::unique_ptr<rtc::QueuedTask> task,
      std::unique_ptr<rtc::QueuedTask> reply,
      const scoped_refptr<base::SequencedTaskRunner>& reply_task_runner)
      : task_(std::move(task)),
        reply_(std::move(reply)),
        reply_task_runner_(reply_task_runner) {}

  ~PostAndReplyTask2() override {}

 private:
  bool Run() override {
    // TODO: Handle Current();
    if (!task_->Run())
      task_.release();

    reply_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&RunTask, base::Passed(&reply_)));
    return true;
  }

  std::unique_ptr<rtc::QueuedTask> task_;
  std::unique_ptr<rtc::QueuedTask> reply_;
  scoped_refptr<base::SequencedTaskRunner> reply_task_runner_;
};

// A lazily created thread local storage for quick access to a TaskQueue.
base::LazyInstance<base::ThreadLocalPointer<rtc::TaskQueue>>::Leaky
    lazy_tls_ptr = LAZY_INSTANCE_INITIALIZER;

void RunTask2(rtc::TaskQueue* tq, std::unique_ptr<rtc::QueuedTask> task) {
  auto* prev = lazy_tls_ptr.Pointer()->Get();
  lazy_tls_ptr.Pointer()->Set(tq);
  if (!task->Run())
    task.release();
  lazy_tls_ptr.Pointer()->Set(prev);
}

}  // namespace

namespace rtc {

bool TaskQueue::IsCurrent() const {
  return Current() == this;
}

class TaskQueue::Impl : public RefCountInterface, public base::Thread {
 public:
  Impl(const char* queue_name, TaskQueue* queue);
  ~Impl() override;

  void InitializeRunner(const base::TaskTraits& traits) {
    DCHECK(!task_runner_);
    task_runner_ = base::CreateSequencedTaskRunnerWithTraits(traits);
  }

  void PostTask(std::unique_ptr<QueuedTask> task) {
    if (task_runner_) {
      // TODO: figure out a safe way for setting the current queue.
      task_runner_->PostTask(
          FROM_HERE, base::BindOnce(&RunTask2, queue_, base::Passed(&task)));
      return;
    }
    task_runner()->PostTask(FROM_HERE,
                            base::BindOnce(&RunTask, base::Passed(&task)));
  }

  void PostDelayedTask(std::unique_ptr<QueuedTask> task,
                       uint32_t milliseconds) {
    if (task_runner_) {
      // TODO: Handle Current();
      task_runner_->PostDelayedTask(
          FROM_HERE, base::BindOnce(&RunTask2, queue_, base::Passed(&task)),
          base::TimeDelta::FromMilliseconds(milliseconds));
      return;
    }
    task_runner()->PostDelayedTask(
        FROM_HERE, base::BindOnce(&RunTask, base::Passed(&task)),
        base::TimeDelta::FromMilliseconds(milliseconds));
  }

  void PostTaskAndReply(std::unique_ptr<QueuedTask> task,
                        std::unique_ptr<QueuedTask> reply,
                        TaskQueue* reply_queue) {
    std::unique_ptr<QueuedTask> t;
    if (reply_queue->impl_->task_runner_) {
      t = std::unique_ptr<QueuedTask>(new PostAndReplyTask2(
          std::move(task), std::move(reply), reply_queue->impl_->task_runner_));
    } else {
      t = std::unique_ptr<QueuedTask>(
          new PostAndReplyTask(std::move(task), std::move(reply),
                               reply_queue->impl_->task_runner()));
    }

    PostTask(std::move(t));
  }

  void PostTaskAndReply(std::unique_ptr<QueuedTask> task,
                        std::unique_ptr<QueuedTask> reply) {
    if (task_runner_) {
      // TODO: Handle Current();
      task_runner_->PostTaskAndReply(
          FROM_HERE, base::BindOnce(&RunTask, base::Passed(&task)),
          base::BindOnce(&RunTask, base::Passed(&reply)));
      return;
    }
    task_runner()->PostTaskAndReply(
        FROM_HERE, base::BindOnce(&RunTask, base::Passed(&task)),
        base::BindOnce(&RunTask, base::Passed(&reply)));
  }

 private:
  virtual void Init() override;

  TaskQueue* const queue_;
  ::scoped_refptr<base::SequencedTaskRunner> task_runner_;
};

TaskQueue::Impl::Impl(const char* queue_name, TaskQueue* queue)
    : base::Thread(queue_name), queue_(queue) {}

void TaskQueue::Impl::Init() {
  lazy_tls_ptr.Pointer()->Set(queue_);
}

TaskQueue::Impl::~Impl() {
  DCHECK(!Thread::IsRunning());
}

TaskQueue::TaskQueue(const char* queue_name,
                     Priority priority /*= Priority::NORMAL*/)
    : impl_(new RefCountedObject<Impl>(queue_name, this)) {
  DCHECK(queue_name);
  // base::Thread::Options options;
  switch (priority) {
    case Priority::HIGH:
#if defined(OS_ANDROID)
      impl_->InitializeRunner(
          {base::WithBaseSyncPrimitives(), base::TaskPriority::HIGHEST});
#else
      impl_->InitializeRunner({base::TaskPriority::HIGHEST});
#endif
      // options.priority = base::ThreadPriority::REALTIME_AUDIO;
      break;
    case Priority::LOW:
      impl_->InitializeRunner(
          {base::MayBlock(), base::TaskPriority::BACKGROUND});
      // options.priority = base::ThreadPriority::BACKGROUND;
      break;
    case Priority::NORMAL:
    default:
#if defined(OS_ANDROID)
      impl_->InitializeRunner({base::WithBaseSyncPrimitives()});
#else
      impl_->InitializeRunner({});
#endif
      // options.priority = base::ThreadPriority::NORMAL;
      break;
  }
  // CHECK(impl_->StartWithOptions(options));
}

TaskQueue::~TaskQueue() {
  DCHECK(!IsCurrent());
  if (impl_->IsRunning())
    impl_->Stop();
}

// static
TaskQueue* TaskQueue::Current() {
  return lazy_tls_ptr.Pointer()->Get();
}

void TaskQueue::PostTask(std::unique_ptr<QueuedTask> task) {
  impl_->PostTask(std::move(task));
}

void TaskQueue::PostDelayedTask(std::unique_ptr<QueuedTask> task,
                                uint32_t milliseconds) {
  impl_->PostDelayedTask(std::move(task), milliseconds);
}

void TaskQueue::PostTaskAndReply(std::unique_ptr<QueuedTask> task,
                                 std::unique_ptr<QueuedTask> reply,
                                 TaskQueue* reply_queue) {
  impl_->PostTaskAndReply(std::move(task), std::move(reply), reply_queue);
}

void TaskQueue::PostTaskAndReply(std::unique_ptr<QueuedTask> task,
                                 std::unique_ptr<QueuedTask> reply) {
  impl_->PostTaskAndReply(std::move(task), std::move(reply));
}

}  // namespace rtc
