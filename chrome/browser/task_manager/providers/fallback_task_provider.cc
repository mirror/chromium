// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/task_manager/providers/fallback_task_provider.h"

#include "base/process/process.h"
#include "base/stl_util.h"
#include "chrome/browser/task_manager/providers/render_process_host_task_provider.h"
#include "chrome/browser/task_manager/providers/web_contents/web_contents_task_provider.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace task_manager {

namespace {

// Returns a task that is in the vector if the task in the vector shares a Pid
// with the other task.
Task* GetTaskByPidFromVector(base::ProcessId process_id,
                             std::vector<Task*>* which_vector) {
  for (Task* candidate : *which_vector) {
    if (candidate->process_id() == process_id)
      return candidate;
  }
  return nullptr;
}

// Returns a task from a vector if that task is in the vector.
Task* GetTaskFromVector(Task* task, std::vector<Task*>* which_vector) {
  for (Task* candidate : *which_vector) {
    if (candidate == task)
      return candidate;
  }
  return nullptr;
}

}  // namespace

FallbackTaskProvider::FallbackTaskProvider(
    std::unique_ptr<TaskProvider> primary_subprovider,
    std::unique_ptr<TaskProvider> secondary_subprovider)
    : weak_ptr_factory_(this) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  task_providers_.emplace_back(std::move(primary_subprovider));
  task_providers_.emplace_back(std::move(secondary_subprovider));
}

FallbackTaskProvider::~FallbackTaskProvider() {}

Task* FallbackTaskProvider::GetTaskOfUrlRequest(int origin_pid,
                                                int child_id,
                                                int route_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  Task* task_of_url_request;
  for (const auto& provider : task_providers_) {
    task_of_url_request =
        provider->GetTaskOfUrlRequest(origin_pid, child_id, route_id);
    if (std::find(shown_tasks_.begin(), shown_tasks_.end(),
                  task_of_url_request) != shown_tasks_.end())
      return task_of_url_request;
  }
  task_of_url_request =
      task_providers_[0]->GetTaskOfUrlRequest(origin_pid, child_id, route_id);
  if (task_of_url_request)
    return task_of_url_request;
  task_of_url_request =
      task_providers_[1]->GetTaskOfUrlRequest(origin_pid, child_id, route_id);
  return task_of_url_request;
}

void FallbackTaskProvider::StartUpdating() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(shown_tasks_.empty());
  DCHECK(primary_subprovider_.empty());
  DCHECK(secondary_subprovider_.empty());
  primary_observer_ = new FallbackTaskProvider::FallbackHelperObserver(
      this, &primary_subprovider_);
  task_providers_[0]->SetObserver(primary_observer_);
  secondary_observer_ = new FallbackTaskProvider::FallbackHelperObserver(
      this, &secondary_subprovider_);
  task_providers_[1]->SetObserver(secondary_observer_);
}

void FallbackTaskProvider::StopUpdating() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  task_providers_[0]->ClearObserver();
  task_providers_[1]->ClearObserver();

  // Then delete all tasks and free memory.
  shown_tasks_.clear();
  secondary_subprovider_.clear();
  primary_subprovider_.clear();
  task_providers_.clear();
  delete primary_observer_;
  delete secondary_observer_;
}

void FallbackTaskProvider::ShowTask(Task* task) {
  shown_tasks_.push_back(task);
  NotifyObserverTaskAdded(task);
}

void FallbackTaskProvider::HideTask(Task* task) {
  base::Erase(shown_tasks_, task);
  NotifyObserverTaskRemoved(task);
}

void FallbackTaskProvider::ManageShowTasks(Task* task,
                                           std::vector<Task*>* which_vector) {
  // If a secondary task is added but a primary task is already shown for it we
  // can ignore showing the secondary.
  if (which_vector == &secondary_subprovider_) {
    if (GetTaskByPidFromVector(task->process_id(), &primary_subprovider_))
      return;
  }

  // If we get a primary task that has a secondary task that is both known and
  // shown hide the secondary task and then show the primary task.
  if (which_vector == &primary_subprovider_) {
    if (Task* secondary_task = GetTaskByPidFromVector(
            task->process_id(), &secondary_subprovider_)) {
      if (Task* secondary_task_shown =
              GetTaskFromVector(secondary_task, &shown_tasks_)) {
        HideTask(secondary_task_shown);
      }
    }
  }
  ShowTask(task);
}

void FallbackTaskProvider::ManageHideTasks(Task* task,
                                           std::vector<Task*>* which_vector) {
  // If a task from the primary subprovider is removed check that if there is
  // not any other primary task and there is secondary task to show the
  // secondary task should be shown.
  if (which_vector == &primary_subprovider_) {
    Task* primary_task =
        GetTaskByPidFromVector(task->process_id(), &primary_subprovider_);
    Task* secondary_task =
        GetTaskByPidFromVector(task->process_id(), &secondary_subprovider_);
    if (secondary_task != nullptr && primary_task == nullptr) {
      ShowTask(secondary_task);
    }
  }
  HideTask(task);
}

void FallbackTaskProvider::TaskAdded(Task* task,
                                     std::vector<Task*>* which_vector) {
  DCHECK(task);
  which_vector->push_back(task);
  ManageShowTasks(task, which_vector);
}

void FallbackTaskProvider::TaskRemoved(Task* task,
                                       std::vector<Task*>* which_vector) {
  DCHECK(task);

  base::Erase(*which_vector, task);
  ManageHideTasks(task, which_vector);
}

void FallbackTaskProvider::TaskUnresponsive(Task* task) {
  DCHECK(task);
  NotifyObserverTaskUnresponsive(task);
}

FallbackTaskProvider::FallbackHelperObserver::FallbackHelperObserver(
    FallbackTaskProvider* fallback_task_provider,
    std::vector<Task*>* which_vector) {
  fallback_task_provider_ = fallback_task_provider;
  fallback_tasks_vector_ = which_vector;
}

FallbackTaskProvider::FallbackHelperObserver::~FallbackHelperObserver() {}

void FallbackTaskProvider::FallbackHelperObserver::TaskAdded(Task* task) {
  DCHECK(task);

  fallback_task_provider_->TaskAdded(task, fallback_tasks_vector_);
}

void FallbackTaskProvider::FallbackHelperObserver::TaskRemoved(Task* task) {
  DCHECK(task);

  fallback_task_provider_->TaskRemoved(task, fallback_tasks_vector_);
}

void FallbackTaskProvider::FallbackHelperObserver::TaskUnresponsive(
    Task* task) {
  fallback_task_provider_->TaskUnresponsive(task);
}

}  // namespace task_manager
