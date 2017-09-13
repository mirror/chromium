// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/task_manager/sampling/task_manager_impl.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "base/command_line.h"
#include "base/containers/adapters.h"
#include "base/task_scheduler/post_task.h"
#include "build/build_config.h"
#include "chrome/browser/task_manager/providers/browser_process_task_provider.h"
#include "chrome/browser/task_manager/providers/child_process_task_provider.h"
#include "chrome/browser/task_manager/providers/render_process_host_task_provider.h"
#include "chrome/browser/task_manager/providers/web_contents/web_contents_task_provider.h"
#include "chrome/browser/task_manager/sampling/shared_sampler.h"
#include "chrome/common/chrome_switches.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/gpu_data_manager.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/task_manager/providers/arc/arc_process_task_provider.h"
#include "components/arc/arc_util.h"
#endif  // defined(OS_CHROMEOS)

namespace task_manager {

namespace {

base::LazyInstance<TaskManagerImpl>::DestructorAtExit
    lazy_task_manager_instance = LAZY_INSTANCE_INITIALIZER;

// Deletes each |TaskGroup| exactly once, even if |container| lists them
// multiple times.
void DeleteAllTaskGroups(std::vector<TaskGroup*>* container) {
  // Use sort/unique to deduplicate pointers.
  std::sort(container->begin(), container->end());
  auto unique_end = std::unique(container->begin(), container->end());
  for (auto it = container->begin(); it != unique_end; ++it) {
    delete *it;
  }
  container->clear();
}

}  // namespace

TaskManagerImpl::TaskManagerImpl()
    : on_background_data_ready_callback_(
          base::Bind(&TaskManagerImpl::OnTaskGroupBackgroundCalculationsDone,
                     base::Unretained(this))),
      blocking_pool_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})),
      shared_sampler_(new SharedSampler(blocking_pool_runner_)),
      is_running_(false),
      weak_ptr_factory_(this) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  task_providers_.emplace_back(new BrowserProcessTaskProvider());
  task_providers_.emplace_back(new ChildProcessTaskProvider());
  task_providers_.emplace_back(new WebContentsTaskProvider());
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kTaskManagerShowExtraRenderers)) {
    task_providers_.emplace_back(new RenderProcessHostTaskProvider());
  }

#if defined(OS_CHROMEOS)
  if (arc::IsArcAvailable())
    task_providers_.emplace_back(new ArcProcessTaskProvider());
#endif  // defined(OS_CHROMEOS)
}

TaskManagerImpl::~TaskManagerImpl() {
}

// static
TaskManagerImpl* TaskManagerImpl::GetInstance() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  return lazy_task_manager_instance.Pointer();
}

void TaskManagerImpl::ActivateTask(TaskId task_id) {
  GetTaskByTaskId(task_id)->Activate();
}

bool TaskManagerImpl::IsTaskKillable(TaskId task_id) {
  return GetTaskByTaskId(task_id)->IsKillable();
}

void TaskManagerImpl::KillTask(TaskId task_id) {
  GetTaskByTaskId(task_id)->Kill();
}

double TaskManagerImpl::GetCpuUsage(TaskId task_id) const {
  return GetTaskGroupByTaskId(task_id)->cpu_usage();
}

base::Time TaskManagerImpl::GetStartTime(TaskId task_id) const {
#if defined(OS_WIN)
  return GetTaskGroupByTaskId(task_id)->start_time();
#else
  NOTIMPLEMENTED();
  return base::Time();
#endif
}

base::TimeDelta TaskManagerImpl::GetCpuTime(TaskId task_id) const {
#if defined(OS_WIN)
  return GetTaskGroupByTaskId(task_id)->cpu_time();
#else
  NOTIMPLEMENTED();
  return base::TimeDelta();
#endif
}

int64_t TaskManagerImpl::GetPhysicalMemoryUsage(TaskId task_id) const {
  return GetTaskGroupByTaskId(task_id)->physical_bytes();
}

int64_t TaskManagerImpl::GetPrivateMemoryUsage(TaskId task_id) const {
  return GetTaskGroupByTaskId(task_id)->private_bytes();
}

int64_t TaskManagerImpl::GetSharedMemoryUsage(TaskId task_id) const {
  return GetTaskGroupByTaskId(task_id)->shared_bytes();
}

int64_t TaskManagerImpl::GetSwappedMemoryUsage(TaskId task_id) const {
#if defined(OS_CHROMEOS)
  return GetTaskGroupByTaskId(task_id)->swapped_bytes();
#else
  return -1;
#endif
}

int64_t TaskManagerImpl::GetGpuMemoryUsage(TaskId task_id,
                                           bool* has_duplicates) const {
  const TaskGroup* task_group = GetTaskGroupByTaskId(task_id);
  if (has_duplicates)
    *has_duplicates = task_group->gpu_memory_has_duplicates();
  return task_group->gpu_memory();
}

base::MemoryState TaskManagerImpl::GetMemoryState(TaskId task_id) const {
  return GetTaskGroupByTaskId(task_id)->memory_state();
}

int TaskManagerImpl::GetIdleWakeupsPerSecond(TaskId task_id) const {
  return GetTaskGroupByTaskId(task_id)->idle_wakeups_per_second();
}

int TaskManagerImpl::GetNaClDebugStubPort(TaskId task_id) const {
#if !defined(DISABLE_NACL)
  return GetTaskGroupByTaskId(task_id)->nacl_debug_stub_port();
#else
  return -2;
#endif  // !defined(DISABLE_NACL)
}

void TaskManagerImpl::GetGDIHandles(TaskId task_id,
                                    int64_t* current,
                                    int64_t* peak) const {
#if defined(OS_WIN)
  const TaskGroup* task_group = GetTaskGroupByTaskId(task_id);
  *current = task_group->gdi_current_handles();
  *peak = task_group->gdi_peak_handles();
#else
  *current = -1;
  *peak = -1;
#endif  // defined(OS_WIN)
}

void TaskManagerImpl::GetUSERHandles(TaskId task_id,
                                     int64_t* current,
                                     int64_t* peak) const {
#if defined(OS_WIN)
  const TaskGroup* task_group = GetTaskGroupByTaskId(task_id);
  *current = task_group->user_current_handles();
  *peak = task_group->user_peak_handles();
#else
  *current = -1;
  *peak = -1;
#endif  // defined(OS_WIN)
}

int TaskManagerImpl::GetOpenFdCount(TaskId task_id) const {
#if defined(OS_LINUX)
  return GetTaskGroupByTaskId(task_id)->open_fd_count();
#else
  return -1;
#endif  // defined(OS_LINUX)
}

bool TaskManagerImpl::IsTaskOnBackgroundedProcess(TaskId task_id) const {
  return GetTaskGroupByTaskId(task_id)->is_backgrounded();
}

const base::string16& TaskManagerImpl::GetTitle(TaskId task_id) const {
  return GetTaskByTaskId(task_id)->title();
}

const std::string& TaskManagerImpl::GetTaskNameForRappor(TaskId task_id) const {
  return GetTaskByTaskId(task_id)->rappor_sample_name();
}

base::string16 TaskManagerImpl::GetProfileName(TaskId task_id) const {
  return GetTaskByTaskId(task_id)->GetProfileName();
}

const gfx::ImageSkia& TaskManagerImpl::GetIcon(TaskId task_id) const {
  return GetTaskByTaskId(task_id)->icon();
}

const base::ProcessHandle& TaskManagerImpl::GetProcessHandle(
    TaskId task_id) const {
  return GetTaskGroupByTaskId(task_id)->process_handle();
}

const base::ProcessId& TaskManagerImpl::GetProcessId(TaskId task_id) const {
  return GetTaskGroupByTaskId(task_id)->process_id();
}

Task::Type TaskManagerImpl::GetType(TaskId task_id) const {
  return GetTaskByTaskId(task_id)->GetType();
}

bool TaskManagerImpl::HasParentTask(TaskId task_id) const {
  return GetTaskByTaskId(task_id)->HasParentTask();
}

const TaskManagerInterface::ProcessSortKey& TaskManagerImpl::GetProcessSortKey(
    TaskId task_id) const {
  return GetTaskGroupByTaskId(task_id)->sort_key();
}

int TaskManagerImpl::GetTabId(TaskId task_id) const {
  return GetTaskByTaskId(task_id)->GetTabId();
}

int TaskManagerImpl::GetChildProcessUniqueId(TaskId task_id) const {
  return GetTaskByTaskId(task_id)->GetChildProcessUniqueID();
}

void TaskManagerImpl::GetTerminationStatus(TaskId task_id,
                                           base::TerminationStatus* out_status,
                                           int* out_error_code) const {
  GetTaskByTaskId(task_id)->GetTerminationStatus(out_status, out_error_code);
}

int64_t TaskManagerImpl::GetNetworkUsage(TaskId task_id) const {
  return GetTaskByTaskId(task_id)->network_usage_rate();
}

int64_t TaskManagerImpl::GetCumulativeNetworkUsage(TaskId task_id) const {
  return GetTaskByTaskId(task_id)->cumulative_network_usage();
}

int64_t TaskManagerImpl::GetProcessTotalNetworkUsage(TaskId task_id) const {
  return GetTaskGroupByTaskId(task_id)->per_process_network_usage_rate();
}

int64_t TaskManagerImpl::GetCumulativeProcessTotalNetworkUsage(
    TaskId task_id) const {
  return GetTaskGroupByTaskId(task_id)->cumulative_per_process_network_usage();
}

int64_t TaskManagerImpl::GetSqliteMemoryUsed(TaskId task_id) const {
  return GetTaskByTaskId(task_id)->GetSqliteMemoryUsed();
}

bool TaskManagerImpl::GetV8Memory(TaskId task_id,
                                  int64_t* allocated,
                                  int64_t* used) const {
  const Task* task = GetTaskByTaskId(task_id);
  if (!task->ReportsV8Memory())
    return false;

  *allocated = task->GetV8MemoryAllocated();
  *used = task->GetV8MemoryUsed();

  return true;
}

bool TaskManagerImpl::GetWebCacheStats(
    TaskId task_id,
    blink::WebCache::ResourceTypeStats* stats) const {
  const Task* task = GetTaskByTaskId(task_id);
  if (!task->ReportsWebCacheStats())
    return false;

  *stats = task->GetWebCacheStats();

  return true;
}

int TaskManagerImpl::GetKeepaliveCount(TaskId task_id) const {
  const Task* task = GetTaskByTaskId(task_id);
  if (!task)
    return -1;

  return task->GetKeepaliveCount();
}

const TaskIdList& TaskManagerImpl::GetTaskIdsList() const {
  DCHECK(is_running_) << "Task manager is not running. You must observe the "
                         "task manager for it to start running";

  return task_ids_;
}

TaskIdList TaskManagerImpl::GetIdsOfTasksSharingSameProcess(
    TaskId task_id) const {
  DCHECK(is_running_) << "Task manager is not running. You must observe the "
                         "task manager for it to start running";

  TaskIdList result;
  TaskGroup* group = GetTaskGroupByTaskId(task_id);
  if (group) {
    result.reserve(group->tasks().size());
    for (Task* task : group->tasks())
      result.push_back(task->task_id());
  }
  return result;
}

size_t TaskManagerImpl::GetNumberOfTasksOnSameProcess(TaskId task_id) const {
  return GetTaskGroupByTaskId(task_id)->num_tasks();
}

TaskId TaskManagerImpl::GetTaskIdForWebContents(
    content::WebContents* web_contents) const {
  if (!web_contents)
    return -1;
  content::RenderFrameHost* rfh = web_contents->GetMainFrame();
  Task* task =
      GetTaskByPidOrRoute(0, rfh->GetProcess()->GetID(), rfh->GetRoutingID());
  if (!task)
    return -1;
  return task->task_id();
}

void TaskManagerImpl::TaskAdded(Task* task) {
  DoAddTask(task);
  NotifyObserversOnTaskAdded(task->task_id());
}

void TaskManagerImpl::TaskRemoved(Task* task) {
  NotifyObserversOnTaskToBeRemoved(task->task_id());
  DoRemoveTask(task);
}

void TaskManagerImpl::TaskReplaced(Task* old_task, Task* new_task) {
  DoAddTask(new_task);
  NotifyObserversOnTaskReplaced(old_task->task_id(), new_task->task_id());
  DoRemoveTask(old_task);
}

void TaskManagerImpl::TaskUnresponsive(Task* task) {
  DCHECK(task);
  NotifyObserversOnTaskUnresponsive(task->task_id());
}

// static
void TaskManagerImpl::OnMultipleBytesTransferredUI(BytesTransferredMap params) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  for (const auto& entry : params) {
    const BytesTransferredKey& process_info = entry.first;
    const BytesTransferredParam& bytes_transferred = entry.second;

    if (!GetInstance()->UpdateTasksWithBytesTransferred(process_info,
                                                        bytes_transferred)) {
      // We can't match a task to the notification.  That might mean the
      // tab that started a download was closed, or the request may have had
      // no originating task associated with it in the first place.
      // We attribute orphaned/unaccounted activity to the Browser process.
      DCHECK(process_info.origin_pid || (process_info.child_id != -1));
      // Since the key is meant to be immutable we create a fake key for the
      // purpose of attributing the orphaned/unaccounted activity to the Browser
      // process.
      int dummy_origin_pid = 0;
      int dummy_child_id = -1;
      int dummy_route_id = -1;
      BytesTransferredKey dummy_key = {dummy_origin_pid, dummy_child_id,
                                       dummy_route_id};
      GetInstance()->UpdateTasksWithBytesTransferred(dummy_key,
                                                     bytes_transferred);
    }
  }
}

void TaskManagerImpl::OnVideoMemoryUsageStatsUpdate(
    const gpu::VideoMemoryUsageStats& gpu_memory_stats) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  gpu_memory_stats_ = gpu_memory_stats;
}

void TaskManagerImpl::Refresh() {
  if (IsResourceRefreshEnabled(REFRESH_TYPE_GPU_MEMORY)) {
    content::GpuDataManager::GetInstance()->RequestVideoMemoryUsageStatsUpdate(
        base::Bind(&TaskManagerImpl::OnVideoMemoryUsageStatsUpdate,
                   weak_ptr_factory_.GetWeakPtr()));
  }

  for (const auto& groups_itr : task_groups_by_proc_id_) {
    groups_itr.second->Refresh(gpu_memory_stats_,
                               GetCurrentRefreshTime(),
                               enabled_resources_flags());
  }

  NotifyObserversOnRefresh(GetTaskIdsList());
}

void TaskManagerImpl::StartUpdating() {
  if (is_running_)
    return;

  is_running_ = true;

  on_background_data_ready_callback_ =
      base::Bind(&TaskManagerImpl::OnTaskGroupBackgroundCalculationsDone,
                 weak_ptr_factory_.GetWeakPtr());

  for (const auto& provider : task_providers_)
    provider->SetObserver(this);

  io_thread_helper_manager_.reset(new IoThreadHelperManager(
      base::BindRepeating(&TaskManagerImpl::OnMultipleBytesTransferredUI)));
}

void TaskManagerImpl::StopUpdating() {
  if (!is_running_)
    return;

  is_running_ = false;

  io_thread_helper_manager_.reset();

  task_groups_by_proc_id_.clear();
  task_ids_.clear();
  DeleteAllTaskGroups(&task_groups_);

  for (const auto& provider : task_providers_)
    provider->ClearObserver();

  weak_ptr_factory_.InvalidateWeakPtrs();
}

Task* TaskManagerImpl::GetTaskByPidOrRoute(int origin_pid,
                                           int child_id,
                                           int route_id) const {
  for (const auto& task_provider : task_providers_) {
    Task* task =
        task_provider->GetTaskOfUrlRequest(origin_pid, child_id, route_id);
    if (task)
      return task;
  }
  return nullptr;
}

bool TaskManagerImpl::UpdateTasksWithBytesTransferred(
    const BytesTransferredKey& key,
    const BytesTransferredParam& param) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  Task* task = GetTaskByPidOrRoute(key.origin_pid, key.child_id, key.route_id);
  if (task) {
    task->OnNetworkBytesRead(param.byte_read_count);
    task->OnNetworkBytesSent(param.byte_sent_count);
    return true;
  }

  // Couldn't match the bytes to any existing task.
  return false;
}

TaskGroup* TaskManagerImpl::GetTaskGroupByTaskId(TaskId task_id) const {
  auto it = std::lower_bound(task_ids_.begin(), task_ids_.end(), task_id);
  DCHECK(it != task_ids_.end());
  DCHECK(*it == task_id);
  return task_groups_[it - task_ids_.begin()];
}

Task* TaskManagerImpl::GetTaskByTaskId(TaskId task_id) const {
  return GetTaskGroupByTaskId(task_id)->GetTaskById(task_id);
}

void TaskManagerImpl::OnTaskGroupBackgroundCalculationsDone() {
  // TODO(afakhry): There should be a better way for doing this!
  bool are_all_processes_data_ready = true;
  for (const auto& groups_itr : task_groups_by_proc_id_) {
    are_all_processes_data_ready &=
        groups_itr.second->AreBackgroundCalculationsDone();
  }
  if (are_all_processes_data_ready) {
    NotifyObserversOnRefreshWithBackgroundCalculations(GetTaskIdsList());
    for (const auto& groups_itr : task_groups_by_proc_id_)
      groups_itr.second->ClearCurrentBackgroundCalculationsFlags();
  }
}

void TaskManagerImpl::DoAddTask(Task* task) {
  DCHECK(task);
  const base::ProcessId proc_id = task->process_id();
  const TaskId task_id = task->task_id();

  TaskGroup* task_group = nullptr;
  auto existing_group = task_groups_by_proc_id_.find(proc_id);
  if (existing_group != task_groups_by_proc_id_.end()) {
    task_group = existing_group->second;
  } else {
    task_group =
        new TaskGroup(task->process_handle(), proc_id, MakeSortKey(task),
                      on_background_data_ready_callback_, shared_sampler_,
                      blocking_pool_runner_);
    if (proc_id != base::kNullProcessId)
      task_groups_by_proc_id_[proc_id] = task_group;
  }

  task_group->AddTask(task);

  DCHECK_EQ(task_groups_.size(), task_ids_.size());

  // Because Tasks are created in order with increasing ID values, |task_id|
  // almost certainly goes onto the back of the |task_ids_| list, so check that
  // first. However, it's possible that a TaskProvider holds onto a Task for a
  // while before registering it, or temporarily unregisters a Task -- in that
  // case, do a binary search.
  auto insertion_index =
      (task_ids_.empty() || task_ids_.back() < task_id)
          ? task_ids_.size()
          : std::lower_bound(task_ids_.begin(), task_ids_.end(), task_id) -
                task_ids_.begin();

  task_ids_.insert(task_ids_.begin() + insertion_index, task_id);
  task_groups_.insert(task_groups_.begin() + insertion_index, task_group);

  DCHECK(std::is_sorted(task_ids_.begin(), task_ids_.end()));
}

void TaskManagerImpl::DoRemoveTask(Task* task) {
  const base::ProcessId proc_id = task->process_id();
  const TaskId task_id = task->task_id();

  DCHECK(proc_id == base::kNullProcessId ||
         task_groups_by_proc_id_.count(proc_id));

  // Remove the Task from the TaskGroup.
  auto index = std::find(task_ids_.begin(), task_ids_.end(), task_id) -
               task_ids_.begin();
  TaskGroup* task_group = task_groups_[index];

  task_ids_.erase(task_ids_.begin() + index);
  task_groups_.erase(task_groups_.begin() + index);

  task_group->RemoveTask(task);

  // If the TaskGroup is now empty, delete it.
  if (task_group->empty()) {
    DCHECK(std::find(task_groups_.begin(), task_groups_.end(), task_group) ==
           task_groups_.end());
    if (proc_id != base::kNullProcessId)
      task_groups_by_proc_id_.erase(proc_id);
    delete task_group;
  }
}

TaskManagerImpl::ProcessSortKey TaskManagerImpl::MakeSortKey(Task* task) const {
  // Compute the sort key for a TaskGroup being created to hold |task|. This is
  // only called when initially creating a TaskGroup.
  if (task->HasParentTask()) {
    // Derive the first part of the sort key from the sort key of the group of
    // the parent task. This will group child processes to be after their
    // parents.
    TaskId parent_id = task->GetParentTask()->task_id();
    TaskGroup* parent_group = GetTaskGroupByTaskId(parent_id);
    TaskGroup::SortKey base_key = parent_group->sort_key();

    // Inherit the first half of the sort key from the parent group -- this
    // ensures that our position is be someplace after it. Among other subtasks
    // of the same parent, our order is determined by the final three
    // tiebreakers: Tab ID, Task type, and finally the TaskId (i.e., Task
    // creation order) itself.
    return std::make_tuple(std::get<0>(base_key), std::get<1>(base_key),
                           std::get<2>(base_key), task->GetTabId(),
                           task->GetType(), task->task_id());

  } else {
    // Generate group ID from task itself. Sort by task type, then by tab ID
    // (which corresponds to tab creation order) and keeps cross-process
    // navigations same-tab.
    return std::make_tuple(task->GetType(), task->GetTabId(), task->task_id(),
                           -1, Task::Type::UNKNOWN, -1);
  }
}

}  // namespace task_manager
