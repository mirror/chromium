// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TASK_MANAGER_SAMPLING_TASK_MANAGER_IMPL_H_
#define CHROME_BROWSER_TASK_MANAGER_SAMPLING_TASK_MANAGER_IMPL_H_

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "base/time/time.h"
#include "chrome/browser/task_manager/providers/task_provider.h"
#include "chrome/browser/task_manager/providers/task_provider_observer.h"
#include "chrome/browser/task_manager/sampling/task_group.h"
#include "chrome/browser/task_manager/sampling/task_manager_io_thread_helper.h"
#include "chrome/browser/task_manager/task_manager_interface.h"
#include "gpu/ipc/common/memory_stats.h"

namespace task_manager {

class SharedSampler;

// Defines a concrete implementation of the TaskManagerInterface.
class TaskManagerImpl : public TaskManagerInterface,
                        public TaskProviderObserver {
 public:
  ~TaskManagerImpl() override;

  static TaskManagerImpl* GetInstance();

  // task_manager::TaskManagerInterface:
  void ActivateTask(TaskId task_id) override;
  bool IsTaskKillable(TaskId task_id) override;
  void KillTask(TaskId task_id) override;
  double GetCpuUsage(TaskId task_id) const override;
  base::Time GetStartTime(TaskId task_id) const override;
  base::TimeDelta GetCpuTime(TaskId task_id) const override;
  int64_t GetPhysicalMemoryUsage(TaskId task_id) const override;
  int64_t GetPrivateMemoryUsage(TaskId task_id) const override;
  int64_t GetSharedMemoryUsage(TaskId task_id) const override;
  int64_t GetSwappedMemoryUsage(TaskId task_id) const override;
  int64_t GetGpuMemoryUsage(TaskId task_id,
                            bool* has_duplicates) const override;
  base::MemoryState GetMemoryState(TaskId task_id) const override;
  int GetIdleWakeupsPerSecond(TaskId task_id) const override;
  int GetNaClDebugStubPort(TaskId task_id) const override;
  void GetGDIHandles(TaskId task_id,
                     int64_t* current,
                     int64_t* peak) const override;
  void GetUSERHandles(TaskId task_id,
                      int64_t* current,
                      int64_t* peak) const override;
  int GetOpenFdCount(TaskId task_id) const override;
  bool IsTaskOnBackgroundedProcess(TaskId task_id) const override;
  const base::string16& GetTitle(TaskId task_id) const override;
  const std::string& GetTaskNameForRappor(TaskId task_id) const override;
  base::string16 GetProfileName(TaskId task_id) const override;
  const gfx::ImageSkia& GetIcon(TaskId task_id) const override;
  const ProcessSortKey& GetProcessSortKey(TaskId task_id) const override;
  const base::ProcessHandle& GetProcessHandle(TaskId task_id) const override;
  const base::ProcessId& GetProcessId(TaskId task_id) const override;
  Task::Type GetType(TaskId task_id) const override;
  bool HasParentTask(TaskId task_id) const override;
  int GetTabId(TaskId task_id) const override;
  int GetChildProcessUniqueId(TaskId task_id) const override;
  void GetTerminationStatus(TaskId task_id,
                            base::TerminationStatus* out_status,
                            int* out_error_code) const override;
  int64_t GetNetworkUsage(TaskId task_id) const override;
  int64_t GetCumulativeNetworkUsage(TaskId task_id) const override;
  int64_t GetProcessTotalNetworkUsage(TaskId task_id) const override;
  int64_t GetCumulativeProcessTotalNetworkUsage(TaskId task_id) const override;
  int64_t GetSqliteMemoryUsed(TaskId task_id) const override;
  bool GetV8Memory(TaskId task_id,
                   int64_t* allocated,
                   int64_t* used) const override;
  bool GetWebCacheStats(
      TaskId task_id,
      blink::WebCache::ResourceTypeStats* stats) const override;
  int GetKeepaliveCount(TaskId task_id) const override;
  const TaskIdList& GetTaskIdsList() const override;
  TaskIdList GetIdsOfTasksSharingSameProcess(TaskId task_id) const override;
  size_t GetNumberOfTasksOnSameProcess(TaskId task_id) const override;
  TaskId GetTaskIdForWebContents(
      content::WebContents* web_contents) const override;

  // task_manager::TaskProviderObserver:
  void TaskAdded(Task* task) override;
  void TaskRemoved(Task* task) override;
  void TaskReplaced(Task* old_task, Task* new_task) override;
  void TaskUnresponsive(Task* task) override;

  // The notification method on the UI thread when multiple bytes are
  // transferred from URLRequests. This will be called by the
  // |io_thread_helper_|
  static void OnMultipleBytesTransferredUI(BytesTransferredMap params);

 private:
  friend struct base::LazyInstanceTraitsBase<TaskManagerImpl>;

  TaskManagerImpl();

  void OnVideoMemoryUsageStatsUpdate(
      const gpu::VideoMemoryUsageStats& gpu_memory_stats);

  // task_manager::TaskManagerInterface:
  void Refresh() override;
  void StartUpdating() override;
  void StopUpdating() override;

  // Lookup a task by its pid, child_id and possibly route_id.
  Task* GetTaskByPidOrRoute(int pid, int child_id, int route_id) const;

  // Based on |param| the appropriate task will be updated by its network usage.
  // Returns true if it was able to match |param| to an existing task, returns
  // false otherwise, at which point the caller must explicitly match these
  // bytes to the browser process by calling this method again with
  // |param.origin_pid = 0| and |param.child_id = param.route_id = -1|.
  bool UpdateTasksWithBytesTransferred(const BytesTransferredKey& key,
                                       const BytesTransferredParam& param);

  TaskGroup* GetTaskGroupByTaskId(TaskId task_id) const;
  Task* GetTaskByTaskId(TaskId task_id) const;

  // Called back by a TaskGroup when the resource calculations done on the
  // background thread has completed.
  void OnTaskGroupBackgroundCalculationsDone();

  // Helpers to add/remove a task from this class's data structures, without
  // notifying observers.
  void DoAddTask(Task* task);
  void DoRemoveTask(Task* task);

  ProcessSortKey MakeSortKey(Task* task) const;

  std::vector<std::unique_ptr<TaskGroup>>::iterator FindExistingGroupPosition(
      TaskGroup* task_group);

  base::Closure on_background_data_ready_callback_;

  // Map TaskGroups by the IDs of the processes they represent.
  std::map<base::ProcessId, TaskGroup*> task_groups_by_proc_id_;

  // Vectors of task IDs and their assigned task groups. These two vectors
  // always have the same number of elements. This is kept as a pair of vectors,
  // rather than as a map, so that we can cheaply access the list of
  // |task_ids_|.
  //
  // |task_ids_| is sorted and can be thought of as the keys. |task_groups_| can
  // be thought of as the values; it may contain duplicates. |task_groups_[i]|
  // gives the group for |task_ids_[i]|.
  std::vector<TaskId> task_ids_;
  std::vector<TaskGroup*> task_groups_;

  // The manager of the IO thread helper used to handle network bytes
  // notifications on IO thread. The manager itself lives on the UI thread, but
  // the IO thread helper lives entirely on the IO thread.
  std::unique_ptr<IoThreadHelperManager> io_thread_helper_manager_;

  // The list of the task providers that are owned and observed by this task
  // manager implementation.
  std::vector<std::unique_ptr<TaskProvider>> task_providers_;

  // The current GPU memory usage stats that was last received from the
  // GpuDataManager.
  gpu::VideoMemoryUsageStats gpu_memory_stats_;

  // The specific blocking pool SequencedTaskRunner that will be used to make
  // sure TaskGroupSampler posts their refreshes serially.
  scoped_refptr<base::SequencedTaskRunner> blocking_pool_runner_;

  // A special sampler shared with all instances of TaskGroup that calculates a
  // subset of resources for all processes at once.
  scoped_refptr<SharedSampler> shared_sampler_;

  // This will be set to true while there are observers and the task manager is
  // running.
  bool is_running_;

  base::WeakPtrFactory<TaskManagerImpl> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(TaskManagerImpl);
};

}  // namespace task_manager

#endif  // CHROME_BROWSER_TASK_MANAGER_SAMPLING_TASK_MANAGER_IMPL_H_
