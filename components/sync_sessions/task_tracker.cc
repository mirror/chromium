// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_sessions/task_tracker.h"

#include <utility>

#include "base/numerics/safe_conversions.h"

namespace sync_sessions {

namespace {

bool DoesTransitionReuseTask(ui::PageTransition transition) {
  return ui::PageTransitionCoreTypeIs(transition, ui::PAGE_TRANSITION_RELOAD) ||
         (transition & ui::PAGE_TRANSITION_FORWARD_BACK);
}

// The criteria for whether a navigation will continue a task chain or start a
// new one.
bool DoesTransitionContinueTask(ui::PageTransition transition) {
  if (ui::PageTransitionCoreTypeIs(transition, ui::PAGE_TRANSITION_LINK) ||
      ui::PageTransitionCoreTypeIs(transition,
                                   ui::PAGE_TRANSITION_AUTO_SUBFRAME) ||
      ui::PageTransitionCoreTypeIs(transition,
                                   ui::PAGE_TRANSITION_MANUAL_SUBFRAME) ||
      ui::PageTransitionCoreTypeIs(transition,
                                   ui::PAGE_TRANSITION_FORM_SUBMIT) ||
      transition & ui::PAGE_TRANSITION_IS_REDIRECT_MASK) {
    return true;
  }
  return false;
}

}  // namespace

TabTasks::TabTasks() {}

TabTasks::TabTasks(const TabTasks& rhs)
    : nav_to_task_id_map_(rhs.nav_to_task_id_map_),
      most_recent_nav_id_(rhs.most_recent_nav_id_) {}

TabTasks::~TabTasks() {}

std::vector<int64_t> TabTasks::GetTaskIdsForNavigation(int nav_id) const {
  std::vector<int64_t> task_id_chain;
  int next_id = nav_id;
  while (next_id != kInvalidNavID &&
         task_id_chain.size() < kMaxNumTasksPerTab) {
    if (nav_to_task_id_map_.count(next_id) == 0)
      break;
    task_id_chain.push_back(nav_to_task_id_map_.at(next_id).task_id);
    next_id = nav_to_task_id_map_.at(next_id).parent_id;
  }

  // Reverse the order so the root is the first item (oldest -> newest).
  std::reverse(task_id_chain.begin(), task_id_chain.end());

  DVLOG(1) << "Returning " << task_id_chain.size() << " task ids for nav "
           << nav_id;
  return task_id_chain;
}

void TabTasks::UpdateWithNavigation(int nav_id,
                                    ui::PageTransition transition,
                                    int64_t global_id) {
  if (nav_to_task_id_map_.count(nav_id) > 0) {
    most_recent_nav_id_ = nav_id;
    return;
  }

  if (DoesTransitionReuseTask(transition)) {
    // Reuse the old task id for reloads or moving back/forward if it's
    // available, else use the current global id.
    if (nav_to_task_id_map_.count(nav_id) == 0)
      nav_to_task_id_map_[nav_id].task_id = global_id;
  } else if (DoesTransitionContinueTask(transition)) {
    nav_to_task_id_map_[nav_id].task_id = global_id;
    if (most_recent_nav_id_ != kInvalidTaskID) {
      nav_to_task_id_map_[nav_id].parent_id = most_recent_nav_id_;
    }
  } else {
    // Otherwise this is a new task.
    nav_to_task_id_map_[nav_id].task_id = global_id;
  }

  DVLOG(1) << "Setting most recent nav id to " << nav_id;
  DVLOG(1) << "Setting current task id to "
           << nav_to_task_id_map_[nav_id].task_id;
  most_recent_nav_id_ = nav_id;

  // Go through and drop the oldest navigations until kMaxNumTasksPerTab
  // navigations remain.
  // TODO(zea): we go through max of 100 iterations here on each new navigation.
  // May be worth attempting to optimize this further if it becomes an issue.
  if (nav_to_task_id_map_.size() > kMaxNumTasksPerTab) {
    int64_t oldest_nav_time = kInvalidTaskID;
    int oldest_nav_id = kInvalidNavID;
    for (auto& iter : nav_to_task_id_map_) {
      if (oldest_nav_id == kInvalidNavID ||
          oldest_nav_time > iter.second.global_id) {
        oldest_nav_id = iter.first;
        oldest_nav_time = iter.second.global_id;
      }
    }

    nav_to_task_id_map_.erase(oldest_nav_id);
    DCHECK_EQ(static_cast<uint64_t>(kMaxNumTasksPerTab),
              nav_to_task_id_map_.size());
  }
}

TaskTracker::TaskTracker() {}

TaskTracker::~TaskTracker() {}

TabTasks* TaskTracker::GetTabTasks(SessionID::id_type tab_id,
                                   SessionID::id_type parent_id) {
  if (local_tab_tasks_map_.count(tab_id) > 0) {
    return local_tab_tasks_map_[tab_id].get();
  }

  if (local_tab_tasks_map_.count(parent_id) > 0) {
    local_tab_tasks_map_[tab_id] =
        base::MakeUnique<TabTasks>(*local_tab_tasks_map_[parent_id]);
  } else {
    local_tab_tasks_map_[tab_id] = base::MakeUnique<TabTasks>();
  }
  return local_tab_tasks_map_[tab_id].get();
}

void TaskTracker::CleanTabTasks(SessionID::id_type tab_id) {
  auto iter = local_tab_tasks_map_.find(tab_id);
  if (iter != local_tab_tasks_map_.end()) {
    local_tab_tasks_map_.erase(iter);
  }
}

}  // namespace sync_sessions
