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
  class SubproviderSource;

  // task_manager::TaskProvider:
  void StartUpdating() override;
  void StopUpdating() override;

  // Creates a FallBackTask from the given |data| and notifies the
  // observer of its addition.
  void ShowTask(Task* task);

  // Deletes a FallBackTask whose |render_process_host_ID| is provided
  // after notifying the observer of its deletion.
  void HideTask(Task* task);

  void TaskUnresponsive(Task* task);

  void OnTaskAddedBySource(Task* task, SubproviderSource* source);
  void OnTaskRemovedBySource(Task* task, SubproviderSource* source);

  SubproviderSource* primary_source() { return sources_[0].get(); }
  SubproviderSource* secondary_source() { return sources_[1].get(); }

  std::unique_ptr<SubproviderSource> sources_[2];

  std::vector<Task*> shown_tasks_;

  // Always keep this the last member of this class to make sure it's the
  // first thing to be destructed.
  base::WeakPtrFactory<FallbackTaskProvider> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(FallbackTaskProvider);
};

class FallbackTaskProvider::SubproviderSource : public TaskProviderObserver {
 public:
  SubproviderSource(FallbackTaskProvider* fallback_task_provider,
                    std::unique_ptr<TaskProvider> subprovider);
  ~SubproviderSource() override;

  TaskProvider* subprovider() { return subprovider_.get(); }
  std::vector<Task*>* tasks() { return &tasks_; }

 private:
  void TaskAdded(Task* task) override;
  void TaskRemoved(Task* task) override;
  void TaskUnresponsive(Task* task) override;

  // The outer task provider on whose behalf we observe the |subprovider_|. This
  // is a pointer back to the class that owns us.
  FallbackTaskProvider* fallback_task_provider_;

  // The task provider that we are observing.
  std::unique_ptr<TaskProvider> subprovider_;

  // The vector of tasks that have been created by |subprovider_|.
  std::vector<Task*> tasks_;
};

}  // namespace task_manager

#endif  // CHROME_BROWSER_TASK_MANAGER_PROVIDERS_FALLBACK_TASK_PROVIDER_H_
