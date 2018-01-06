// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/embedder/embedded_instance_manager.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"

namespace service_manager {

namespace {

void JoinAndReleaseThread(std::unique_ptr<base::Thread> thread) {
  if (!thread)
    return;

  thread.reset();
}

}  // namespace

EmbeddedInstanceManager::EmbeddedInstanceManager(
    const base::StringPiece& name,
    const EmbeddedServiceInfo& info,
    base::RepeatingClosure quit_closure)
    : name_(name.as_string()),
      factory_callback_(info.factory),
      use_dedicated_thread_(!info.task_runner && info.use_own_thread),
      message_loop_type_(info.message_loop_type),
      thread_priority_(info.thread_priority),
      quit_closure_(std::move(quit_closure)),
      owner_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      service_task_runner_(info.task_runner),
      active_and_pending_service_request_count_(0),
      shutdown_has_been_called_(false) {
  CHECK(quit_closure_);
  if (!use_dedicated_thread_ && !service_task_runner_)
    service_task_runner_ = base::ThreadTaskRunnerHandle::Get();
}

EmbeddedInstanceManager::~EmbeddedInstanceManager() {
  DCHECK(!optional_dedicated_service_thread_);
  // if (optional_dedicated_service_thread_) {
  //   // This may happen if the owner shuts down the thread serving
  //   // |owner_task_runner_| before an invocation of
  //   // OnServiceRequestCountChangedToZero() posted to it had the chance to
  //   // execute.

  //   // O: This can lead to the following error:
  //   // [82150:82150:0105/185742.648907:FATAL:thread_restrictions.cc(105)]
  //   Check
  //   // failed:
  //   // !g_base_sync_primitives_disallowed.Get().Get(). Waiting on a //base
  //   sync
  //   // primitive is not allowed on this thread to prevent jank and deadlock.
  //   //
  //   // Looks like this may get executed from a context where posting tasks is
  //   // not allowed.
  //   base::PostTaskWithTraits(
  //       FROM_HERE, {base::TaskPriority::BACKGROUND, base::MayBlock()},
  //       base::BindOnce(&JoinAndReleaseThread,
  //                      base::Passed(&optional_dedicated_service_thread_)));
  // }
}

void EmbeddedInstanceManager::SetEventHandlingCompleteObserver(
    base::RepeatingClosure observer_cb) {
  optional_event_handling_complete_observer_cb_ = std::move(observer_cb);
}

void EmbeddedInstanceManager::BindServiceRequest(
    service_manager::mojom::ServiceRequest request) {
  DCHECK(owner_task_runner_->RunsTasksInCurrentSequence());

  // If this is called after ShutDown(), it may happen that a new
  // dedicated thread gets created while an old one is still running and
  // potentially serving callbacks/tasks. Since this may lead to difficult to
  // interpret symptoms, we put a hard check in place to prevent this.
  CHECK(!shutdown_has_been_called_);

  // Increase this counter to prevent a call to OnInstanceLost() from tearing
  // down our dedicated thread after we create it.
  active_and_pending_service_request_count_++;

  // Start a new thread if necessary.
  if (use_dedicated_thread_ && active_and_pending_service_request_count_ == 1) {
    DCHECK(service_task_runner_ == nullptr);
    optional_dedicated_service_thread_.reset(new base::Thread(name_));
    base::Thread::Options options;
    options.message_loop_type = message_loop_type_;
    options.priority = thread_priority_;
    optional_dedicated_service_thread_->StartWithOptions(options);
    service_task_runner_ = optional_dedicated_service_thread_->task_runner();
  }

  DCHECK(service_task_runner_);
  if (service_task_runner_->RunsTasksInCurrentSequence()) {
    BindServiceRequestOnServiceSequence(std::move(request));
  } else {
    service_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &EmbeddedInstanceManager::BindServiceRequestOnServiceSequence, this,
            base::Passed(&request)));
  }
}

void EmbeddedInstanceManager::ShutDown() {
  DCHECK(owner_task_runner_->RunsTasksInCurrentSequence());
  shutdown_has_been_called_ = true;
  if (!service_task_runner_) {
    // This may happen if either there never was any entry or all entries have
    // already been removed via OnInstanceLost().
    OnEventHandlingComplete();
    return;
  }
  // Any extant ServiceContexts must be destroyed on |service_task_runner_|.
  if (service_task_runner_->RunsTasksInCurrentSequence()) {
    ShutDownOnServiceSequence();
  } else {
    service_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&EmbeddedInstanceManager::ShutDownOnServiceSequence,
                       this));
  }
}

void EmbeddedInstanceManager::BindServiceRequestOnServiceSequence(
    service_manager::mojom::ServiceRequest request) {
  // At this point, |service_task_runner_| is guaranteed to be valid, because
  // the already increased value of |active_and_pending_service_request_count_|
  // guarantees that a corresponding dedicated thread does not get torn down via
  // OnInstanceLost().
  DCHECK(service_task_runner_->RunsTasksInCurrentSequence());
  int instance_id = next_instance_id_++;
  auto context = std::make_unique<service_manager::ServiceContext>(
      factory_callback_.Run(), std::move(request));
  context->SetQuitClosure(base::BindRepeating(
      &EmbeddedInstanceManager::OnInstanceLost, this, instance_id));
  id_to_context_map_.insert(std::make_pair(instance_id, std::move(context)));
}

void EmbeddedInstanceManager::OnInstanceLost(int instance_id) {
  if (!service_task_runner_) {
    // This may happen if the entry has already been cleared by a call to
    // ShutDown().
    OnEventHandlingComplete();
    return;
  }
  DCHECK(service_task_runner_->RunsTasksInCurrentSequence());
  auto context_iter = id_to_context_map_.find(instance_id);
  if (context_iter == id_to_context_map_.end()) {
    // This may happen if the entry has already been cleared by a call to
    // ShutDown().
    OnEventHandlingComplete();
    return;
  }
  id_to_context_map_.erase(context_iter);
  owner_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&EmbeddedInstanceManager::DecreaseServiceRequestCount,
                     this, base::Passed(&optional_dedicated_service_thread_)));
}

void EmbeddedInstanceManager::ShutDownOnServiceSequence() {
  if (!service_task_runner_) {
    // This may happen if either there never was any entry or all entries have
    // already been removed via OnInstanceLost().
    OnEventHandlingComplete();
    return;
  }
  DCHECK(service_task_runner_->RunsTasksInCurrentSequence());
  if (id_to_context_map_.empty()) {
    // This may happen if either there never was any entry or all entries have
    // already been removed via OnInstanceLost().
    OnEventHandlingComplete();
    return;
  }
  id_to_context_map_.clear();
  // If the owner has already released its reference to |this| it may
  // also already have shut down the thread serving |owner_task_runner_|.
  // In that case, the posted task will never get run.
  // Since in the destructor we can neither block to shut down the thread nor
  // post a background task (because using base sync primitives may be
  // disallowed), we have no choice but to leak the thread in that case.
  owner_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&EmbeddedInstanceManager::SetServiceRequestCountToZero,
                     this, base::Passed(&optional_dedicated_service_thread_)));
}

void EmbeddedInstanceManager::DecreaseServiceRequestCount(
    std::unique_ptr<base::Thread> thread) {
  DCHECK(owner_task_runner_->RunsTasksInCurrentSequence());
  active_and_pending_service_request_count_--;
  if (active_and_pending_service_request_count_ == 0)
    OnServiceRequestCountChangedToZero(std::move(thread));
  else {
    // Pass ownership of thread back to |this|.
    optional_dedicated_service_thread_ = std::move(thread);
  }
  OnEventHandlingComplete();
}

void EmbeddedInstanceManager::SetServiceRequestCountToZero(
    std::unique_ptr<base::Thread> thread) {
  DCHECK(owner_task_runner_->RunsTasksInCurrentSequence());
  active_and_pending_service_request_count_ = 0;
  OnServiceRequestCountChangedToZero(std::move(thread));
  OnEventHandlingComplete();
}

void EmbeddedInstanceManager::OnServiceRequestCountChangedToZero(
    std::unique_ptr<base::Thread> thread) {
  DCHECK(owner_task_runner_->RunsTasksInCurrentSequence());
  if (use_dedicated_thread_) {
    base::PostTaskWithTraits(
        FROM_HERE, {base::TaskPriority::BACKGROUND, base::MayBlock()},
        base::BindOnce(&JoinAndReleaseThread, base::Passed(&thread)));
    service_task_runner_ = nullptr;
  }
  quit_closure_.Run();
}

void EmbeddedInstanceManager::OnEventHandlingComplete() {
  if (!owner_task_runner_->RunsTasksInCurrentSequence()) {
    owner_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&EmbeddedInstanceManager::OnEventHandlingComplete,
                       this));
    return;
  }
  if (!optional_event_handling_complete_observer_cb_)
    return;
  optional_event_handling_complete_observer_cb_.Run();
}

}  // namespace service_manager
