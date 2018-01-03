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

EmbeddedInstanceManager::EmbeddedInstanceManager(
    const base::StringPiece& name,
    const EmbeddedServiceInfo& info,
    base::OnceClosure quit_closure)
    : name_(name.as_string()),
      factory_callback_(info.factory),
      use_dedicated_thread_(!info.task_runner && info.use_own_thread),
      message_loop_type_(info.message_loop_type),
      thread_priority_(info.thread_priority),
      quit_closure_(std::move(quit_closure)),
      owner_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      service_task_runner_(info.task_runner) {
  if (!use_dedicated_thread_ && !service_task_runner_)
    service_task_runner_ = base::ThreadTaskRunnerHandle::Get();
}

void EmbeddedInstanceManager::BindServiceRequest(
    service_manager::mojom::ServiceRequest request) {
  DCHECK(owner_task_runner_->RunsTasksInCurrentSequence());

  if (use_dedicated_thread_ && !optional_dedicated_service_thread_) {
    // Start a new thread if necessary.
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
  if (!service_task_runner_)
    return;
  // Any extant ServiceContexts must be destroyed on |service_task_runner_|.
  if (service_task_runner_->RunsTasksInCurrentSequence()) {
    QuitOnServiceSequence(std::move(optional_dedicated_service_thread_));
  } else {
    service_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&EmbeddedInstanceManager::QuitOnServiceSequence, this,
                       std::move(optional_dedicated_service_thread_)));
  }
}

EmbeddedInstanceManager::~EmbeddedInstanceManager() {
  // If this instance had its own thread, it MUST be explicitly destroyed by
  // QuitOnOwnerThread() by the time this destructor is run.
  DCHECK(!optional_dedicated_service_thread_);
}

void EmbeddedInstanceManager::BindServiceRequestOnServiceSequence(
    service_manager::mojom::ServiceRequest request) {
  DCHECK(service_task_runner_->RunsTasksInCurrentSequence());

  int instance_id = next_instance_id_++;
  auto context = std::make_unique<service_manager::ServiceContext>(
      factory_callback_.Run(), std::move(request));
  context->SetQuitClosure(base::BindRepeating(
      &EmbeddedInstanceManager::OnInstanceLost, this, instance_id));
  id_to_context_map_.insert(std::make_pair(instance_id, std::move(context)));
}

void EmbeddedInstanceManager::OnInstanceLost(int instance_id) {
  DCHECK(service_task_runner_->RunsTasksInCurrentSequence());

  auto context_iter = id_to_context_map_.find(instance_id);
  CHECK(context_iter != id_to_context_map_.end());
  id_to_context_map_.erase(context_iter);

  // If we've lost the last instance, run the quit closure.
  if (id_to_context_map_.empty())
    QuitOnServiceSequence(std::move(optional_dedicated_service_thread_));
}

void EmbeddedInstanceManager::QuitOnServiceSequence(
    std::unique_ptr<base::Thread> optional_dedicated_service_thread) {
  DCHECK(service_task_runner_->RunsTasksInCurrentSequence());

  id_to_context_map_.clear();

  if (optional_dedicated_service_thread) {
    scoped_refptr<base::TaskRunner> sequenced_task_runner =
        base::CreateSequencedTaskRunnerWithTraits(
            {base::TaskPriority::BACKGROUND, base::MayBlock()});
    sequenced_task_runner->PostTask(
        FROM_HERE,
        base::BindOnce(
            &EmbeddedInstanceManager::JoinAndReleaseDedicatedServiceThread,
            this, std::move(optional_dedicated_service_thread)));
    sequenced_task_runner->PostTask(
        FROM_HERE,
        base::BindOnce(
            &EmbeddedInstanceManager::RunOrPostQuitClosureOnOwnerTaskRunner,
            this));
  } else {
    RunOrPostQuitClosureOnOwnerTaskRunner();
  }
}

void EmbeddedInstanceManager::JoinAndReleaseDedicatedServiceThread(
    std::unique_ptr<base::Thread> thread) {
  if (!thread)
    return;

  thread.reset();
  service_task_runner_ = nullptr;
}

void EmbeddedInstanceManager::RunOrPostQuitClosureOnOwnerTaskRunner() {
  if (!owner_task_runner_->RunsTasksInCurrentSequence()) {
    owner_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &EmbeddedInstanceManager::RunOrPostQuitClosureOnOwnerTaskRunner,
            this));
    return;
  }

  base::ResetAndReturn(&quit_closure_).Run();
}

}  // namespace service_manager
