// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TASK_MANAGER_PROVIDERS_FALLBACK_TASK_PROVIDER_H_
#define CHROME_BROWSER_TASK_MANAGER_PROVIDERS_FALLBACK_TASK_PROVIDER_H_

#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/task_manager/providers/task_provider.h"
#include "chrome/browser/task_manager/providers/task_provider_observer.h"

namespace task_manager {

// Defines a provider to provide possibly redundant tasks that represent RPHs
// that are active, this can track interstitial pages and tracks RPHs that have
// service workers still alive in them.
class FallbackTaskProvider : public TaskProvider {
 public:
  FallbackTaskProvider(std::unique_ptr<TaskProvider> primary_subprovider,
                       std::unique_ptr<TaskProvider> secondary_subprovider);
  ~FallbackTaskProvider() override;

  // task_manager::TaskProvider:
  Task* GetTaskOfUrlRequest(int origin_pid,
                            int child_id,
                            int route_id) override;

 private:
  class FallbackHelperObserver;
  FallbackHelperObserver* primary_observer_;
  FallbackHelperObserver* secondary_observer_;

  // task_manager::TaskProvider:
  void StartUpdating() override;
  void StopUpdating() override;

  // Creates a FallBackTask from the given |data| and notifies the
  // observer of its addition.
  void ShowTask(Task* task);

  // Deletes a FallBackTask whose |render_process_host_ID| is provided
  // after notifying the observer of its deletion.
  void HideTask(Task* task);

  void TaskAdded(Task* task, std::vector<Task*>* which_vector);
  void TaskRemoved(Task* task, std::vector<Task*>* which_vector);
  void TaskUnresponsive(Task* task);

  void ManageShowTasks(Task* task, std::vector<Task*>* which_vector);
  void ManageHideTasks(Task* task, std::vector<Task*>* which_vector);

  std::vector<Task*> primary_subprovider_;
  std::vector<Task*> secondary_subprovider_;
  std::vector<Task*> shown_tasks_;

  // The list of the task providers that are owned and observed by this task
  // manager implementation.
  std::vector<std::unique_ptr<TaskProvider>> task_providers_;

  // Always keep this the last member of this class to make sure it's the
  // first thing to be destructed.
  base::WeakPtrFactory<FallbackTaskProvider> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(FallbackTaskProvider);
};

class FallbackTaskProvider::FallbackHelperObserver
    : public TaskProviderObserver {
 public:
  FallbackHelperObserver(FallbackTaskProvider* fallback_task_provider,
                         std::vector<Task*>* which_vector);
  ~FallbackHelperObserver() override;

 private:
  void TaskAdded(Task* task) override;
  void TaskRemoved(Task* task) override;
  void TaskUnresponsive(Task* task) override;
  FallbackTaskProvider* fallback_task_provider_;
  std::vector<Task*>* fallback_tasks_vector_;
};

}  // namespace task_manager

#endif  // CHROME_BROWSER_TASK_MANAGER_PROVIDERS_FALLBACK_TASK_PROVIDER_H_
