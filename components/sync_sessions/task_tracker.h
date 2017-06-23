// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_SESSIONS_TASK_TRACKER_H_
#define COMPONENTS_SYNC_SESSIONS_TASK_TRACKER_H_

#include <stddef.h>

#include <map>
#include <memory>
#include <vector>

#include "base/time/clock.h"
#include "base/time/default_clock.h"
#include "base/time/time.h"
#include "components/sessions/core/session_id.h"
#include "components/sessions/core/session_types.h"
#include "ui/base/page_transition_types.h"

namespace sync_sessions {

// Class to generate and manage task ids for navigations of a tab.
//
// TODO(shenchao): If the tab is restored, then the input navigation is not
// necessarily the first navigation in this case. Need to fix it by initializing
// the object with restored data.
class TabTasks {
 public:
  static const int64_t kInvalidTaskID = -1;
  static const int kInvalidNavID = -1;

  TabTasks();
  explicit TabTasks(const TabTasks& rhs);
  virtual ~TabTasks();

  // Gets root->leaf task id list for the navigation denoted by |nav_id|.
  // Returns an empty vector if |nav_id| is not found.
  std::vector<int64_t> GetTaskIdsForNavigation(int nav_id) const;

  void UpdateWithNavigation(int nav_id,
                            ui::PageTransition transition,
                            int64_t global_id);

 private:
  FRIEND_TEST_ALL_PREFIXES(TaskTrackerTest, LimitMaxNumberOfTasksPerTab);
  FRIEND_TEST_ALL_PREFIXES(TaskTrackerTest,
                           CreateSubTaskFromExcludedAncestorTask);

  // The maximum number of tasks we track in a tab.
  static const int kMaxNumTasksPerTab = 100;

  struct TaskIdAndParent {
    // The task id for this navigation. Should always be present.
    int64_t task_id = kInvalidTaskID;

    // The most recent global id for this task. This is used to determine the
    // age of this navigation when expiring old navigations.
    int64_t global_id = kInvalidTaskID;

    // If present, the nav id that this task is a continuation of. If this is
    // the first navigation in a new task, may not be present.
    int parent_id = kInvalidNavID;
  };

  // Map converting navigation ids to a TaskIdAndParent. To find the root to
  // leaf chain for a navigation, start with its navigation id, and reindex into
  // the map using the parent id, until the parent id is kInvalidNavID.
  std::map<int, TaskIdAndParent> nav_to_task_id_map_;

  // The most recent navigation id seen for this tab.
  int most_recent_nav_id_ = kInvalidNavID;
};

// Tracks tasks of current session.
class TaskTracker {
 public:
  // Constructs with a clock to get timestamp as new task ids.
  TaskTracker();
  virtual ~TaskTracker();

  // Returns a TabTasks pointer, which is owned by this object, for the tab of
  // given |tab_id|.
  TabTasks* GetTabTasks(SessionID::id_type tab_id,
                        SessionID::id_type parent_id);

  // Cleans tracked task ids of navigations in the tab of |tab_id|.
  void CleanTabTasks(SessionID::id_type tab_id);

 private:
  FRIEND_TEST_ALL_PREFIXES(TaskTrackerTest, CleanTabTasks);

  std::map<SessionID::id_type, std::unique_ptr<TabTasks>> local_tab_tasks_map_;

  DISALLOW_COPY_AND_ASSIGN(TaskTracker);
};

}  // namespace sync_sessions

#endif  // COMPONENTS_SYNC_SESSIONS_TASK_TRACKER_H_
