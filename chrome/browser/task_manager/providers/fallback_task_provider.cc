// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/task_manager/providers/fallback_task_provider.h"

#include "base/process/process.h"
#include "chrome/browser/task_manager/providers/child_process_task.h"
#include "chrome/browser/task_manager/providers/render_process_host_task_provider.h"
#include "chrome/browser/task_manager/providers/web_contents/web_contents_task_provider.h"
#include "content/public/browser/browser_child_process_host_iterator.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_data.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/process_type.h"
#include "extensions/features/features.h"

using content::RenderProcessHost;
using content::BrowserThread;

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/browser/process_map.h"
#endif

namespace task_manager {

namespace {
void AddTaskToVector(Task* task, std::vector<Task*>* which_vector) {
  unsigned long i;
  for (i = 0; i < which_vector->size(); i++) {
    if ((*which_vector)[i] == task) {
      return;
    }
  }

  which_vector->push_back(task);
}

void RemoveTaskFromVector(Task* task, std::vector<Task*>* which_vector) {
  unsigned long i;
  for (i = 0; i < which_vector->size(); i++) {
    if ((*which_vector)[i] == task) {
      break;
    }
  }

  if (i < which_vector->size()) {
    which_vector->erase(which_vector->begin() + i);
  }
}

Task* GetTaskByPidFromVector(Task* task, std::vector<Task*>* which_vector) {
  unsigned long i;
  for (i = 0; i < which_vector->size(); i++) {
    if ((*which_vector)[i]->process_id() == task->process_id()) {
      return (*which_vector)[i];
    }
  }
  return nullptr;
}
Task* GetTaskFromVector(Task* task, std::vector<Task*>* which_vector) {
  unsigned long i;
  for (i = 0; i < which_vector->size(); i++) {
    if ((*which_vector)[i] == task) {
      return (*which_vector)[i];
    }
  }
  return nullptr;
}
}  // namespace
FallbackTaskProvider::FallbackTaskProvider() : weak_ptr_factory_(this) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  task_providers_.emplace_back(new WebContentsTaskProvider());
  task_providers_.emplace_back(new RenderProcessHostTaskProvider());
}

FallbackTaskProvider::~FallbackTaskProvider() {}

Task* FallbackTaskProvider::GetTaskOfUrlRequest(int origin_pid,
                                                int child_id,
                                                int route_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  Task* task_of_url_request;
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
  DCHECK(tasks_by_rph_id_.empty());
  DCHECK(tasks_by_pid_.empty());

  wctpobserver_ = new FallbackTaskProvider::WCTPObserver(this, &wctptasks_);
  task_providers_[0]->SetObserver(wctpobserver_);  // WCTPO
  rphtpobserver_ = new FallbackTaskProvider::RPHTPObserver(this, &rphtptasks_);
  task_providers_[1]->SetObserver(rphtpobserver_);  // RPHTPO
}

void FallbackTaskProvider::StopUpdating() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  for (const auto& provider : task_providers_)
    provider->ClearObserver();

  weak_ptr_factory_.InvalidateWeakPtrs();

  // Then delete all tasks (if any).
  tasks_by_rph_id_.clear();
  tasks_by_pid_.clear();
}

void FallbackTaskProvider::CreateTask(Task* task) {
  if (GetTaskFromVector(task, &shown_tasks_))
    return;

  AddTaskToVector(task, &shown_tasks_);
  NotifyObserverTaskAdded(task);
}

void FallbackTaskProvider::DeleteTask(Task* task) {
  Task* shown_task = GetTaskFromVector(task, &shown_tasks_);
  if (!shown_task) {
    return;
  }
  RemoveTaskFromVector(task, &shown_tasks_);
  NotifyObserverTaskRemoved(task);
}

void FallbackTaskProvider::ManageAddTasks(Task* task,
                                          std::vector<Task*>* which_vector) {
  if (which_vector == &rphtptasks_) {
    if (GetTaskByPidFromVector(task, &wctptasks_))
      return;
  }
  if (which_vector == &wctptasks_) {
    if (Task* rpht = GetTaskByPidFromVector(task, &rphtptasks_)) {
      if (Task* rpht_shown = GetTaskFromVector(rpht, &shown_tasks_)) {
        DeleteTask(rpht_shown);
      }
    }
  }
  CreateTask(task);
}

void FallbackTaskProvider::ManageRemoveTasks(Task* task,
                                             std::vector<Task*>* which_vector) {
  if (which_vector == &wctptasks_) {
    Task* other_wct = GetTaskByPidFromVector(task, &wctptasks_);
    Task* rpht = GetTaskByPidFromVector(task, &rphtptasks_);
    if (rpht != nullptr && other_wct == nullptr) {
      CreateTask(rpht);
    }
  }
  DeleteTask(task);
}

void FallbackTaskProvider::TaskAdded(Task* task,
                                     std::vector<Task*>* which_vector) {
  DCHECK(task);

  AddTaskToVector(task, which_vector);
  ManageAddTasks(task, which_vector);
}

void FallbackTaskProvider::TaskRemoved(Task* task,
                                       std::vector<Task*>* which_vector) {
  DCHECK(task);

  RemoveTaskFromVector(task, which_vector);
  ManageRemoveTasks(task, which_vector);
}

void FallbackTaskProvider::TaskUnresponsive(Task* task) {
  DCHECK(task);
}

FallbackTaskProvider::WCTPObserver::WCTPObserver(
    FallbackTaskProvider* fallback_task_provider,
    std::vector<Task*>* which_vector) {
  fallback_task_provider_ = fallback_task_provider;
  wct_vector_ = which_vector;
}

FallbackTaskProvider::WCTPObserver::~WCTPObserver() {}

void FallbackTaskProvider::WCTPObserver::TaskAdded(Task* task) {
  DCHECK(task);

  fallback_task_provider_->TaskAdded(task, wct_vector_);
}

void FallbackTaskProvider::WCTPObserver::TaskRemoved(Task* task) {
  DCHECK(task);

  fallback_task_provider_->TaskRemoved(task, wct_vector_);
}

void FallbackTaskProvider::WCTPObserver::TaskUnresponsive(Task* task) {}

FallbackTaskProvider::RPHTPObserver::RPHTPObserver(
    FallbackTaskProvider* fallback_task_provider,
    std::vector<Task*>* which_vector) {
  fallback_task_provider_ = fallback_task_provider;
  rpht_vector_ = which_vector;
}

FallbackTaskProvider::RPHTPObserver::~RPHTPObserver() {}
void FallbackTaskProvider::RPHTPObserver::TaskAdded(Task* task) {
  DCHECK(task);

  fallback_task_provider_->TaskAdded(task, rpht_vector_);
}

void FallbackTaskProvider::RPHTPObserver::TaskRemoved(Task* task) {
  DCHECK(task);

  fallback_task_provider_->TaskRemoved(task, rpht_vector_);
}

void FallbackTaskProvider::RPHTPObserver::TaskUnresponsive(Task* task) {}

}  // namespace task_manager
