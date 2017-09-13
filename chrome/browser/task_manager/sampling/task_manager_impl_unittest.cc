// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/task_manager/providers/task.h"
#include "chrome/browser/task_manager/sampling/task_manager_impl.h"
#include "chrome/browser/task_manager/task_manager_observer.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace task_manager {

namespace {

// A Task for unittests, not backed by a real process, that can report any given
// value.
class FakeTask : public Task {
 public:
  FakeTask(base::ProcessId process_id,
           Type type,
           const std::string& title,
           int tab_id,
           Task* parent)
      : Task(base::ASCIIToUTF16(title),
             "FakeTask",
             nullptr,
             base::kNullProcessHandle,
             process_id),
        type_(type),
        parent_(parent),
        tab_id_(tab_id) {
    TaskManagerImpl::GetInstance()->TaskAdded(this);
  }

  ~FakeTask() override { TaskManagerImpl::GetInstance()->TaskRemoved(this); }

  Type GetType() const override { return type_; }

  int GetChildProcessUniqueID() const override { return 0; }

  const Task* GetParentTask() const override { return parent_; }

  int GetTabId() const override { return tab_id_; }

  // Updates the parent after the fact. Because TaskManagerImpl currently only
  // calls GetParentTask() as part of TaskAdded, this should have no effect
  // other than to validate that such calls do not occur.
  void SetParent(Task* parent) { parent_ = parent; }

 private:
  Type type_;
  Task* parent_;
  int tab_id_;

  DISALLOW_COPY_AND_ASSIGN(FakeTask);
};

}  // namespace

class TaskManagerImplTest : public testing::Test, public TaskManagerObserver {
 public:
  TaskManagerImplTest()
      : TaskManagerObserver(base::TimeDelta::FromSeconds(1),
                            REFRESH_TYPE_NONE) {
    TaskManagerImpl::GetInstance()->AddObserver(this);
  }
  ~TaskManagerImplTest() override {
    tasks_.clear();
    observed_task_manager()->RemoveObserver(this);
  }

  FakeTask* AddTask(int pid_offset,
                    Task::Type type,
                    const std::string& title,
                    int tab_id,
                    Task* parent = nullptr) {
    // Offset based on the current process id, to avoid collisions with the
    // browser process task.
    base::ProcessId process_id = base::GetCurrentProcId() + pid_offset;
    tasks_.emplace_back(new FakeTask(process_id, type, title, tab_id, parent));
    return tasks_.back().get();
  }

  void RemoveTask(FakeTask* task) {
    for (size_t i = 0; i < tasks_.size(); ++i) {
      if (tasks_[i].get() == task) {
        tasks_.erase(tasks_.begin() + i);
        return;
      }
    }
  }

  std::string DumpSortedTasks() {
    std::string result;

    TaskManagerInterface* task_manager = observed_task_manager();
    std::vector<TaskId> sorted_task_ids = task_manager->GetTaskIdsList();
    std::stable_sort(
        sorted_task_ids.begin(), sorted_task_ids.end(),
        [task_manager](const TaskId& a, const TaskId& b) {
          return std::make_tuple(task_manager->GetProcessSortKey(a),
                                 task_manager->HasParentTask(a),
                                 task_manager->GetTabId(a)) <
                 std::make_tuple(task_manager->GetProcessSortKey(b),
                                 task_manager->HasParentTask(b),
                                 task_manager->GetTabId(b));
        });

    // Other than the FakeTasks we've created, the TaskManager should only
    // contain the Browser process.
    EXPECT_EQ(tasks_.size() + 1, sorted_task_ids.size());

    for (TaskId task_id : sorted_task_ids) {
      result += base::UTF16ToUTF8(observed_task_manager()->GetTitle(task_id));
      result += "\n";
    }
    return result;
  }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  std::vector<std::unique_ptr<FakeTask>> tasks_;
  DISALLOW_COPY_AND_ASSIGN(TaskManagerImplTest);
};

TEST_F(TaskManagerImplTest, SortingTypes) {
  AddTask(100, Task::GPU, "Gpu Process", -1);

  Task* tab1 = AddTask(200, Task::RENDERER, "Tab One", 10);
  AddTask(400, Task::EXTENSION, "Extension Subframe: Tab One", 10, tab1);
  AddTask(300, Task::RENDERER, "Subframe: Tab One", 10, tab1);

  Task* tab2 =
      AddTask(200, Task::RENDERER, "Tab Two: sharing process with Tab One", 20);

  AddTask(301, Task::RENDERER, "Subframe: Tab Two", 20, tab2);
  AddTask(400, Task::EXTENSION, "Extension Subframe: Tab Two", 20, tab2);

  AddTask(600, Task::ARC, "ARC", -1);
  AddTask(800, Task::UTILITY, "Utility One", -1);
  AddTask(700, Task::UTILITY, "Utility Two", -1);
  AddTask(1000, Task::GUEST, "Guest", 20);
  AddTask(900, Task::WORKER, "Worker", -1);
  AddTask(500, Task::ZYGOTE, "Zygote", -1);

  AddTask(300, Task::RENDERER, "Subframe: Tab One (2)", 10, tab1);
  AddTask(300, Task::RENDERER, "Subframe: Tab One (third)", 10, tab1);
  AddTask(300, Task::RENDERER, "Subframe: Tab One (4)", 10, tab1);

  EXPECT_EQ(
      "Browser\n"
      "Gpu Process\n"
      "ARC\n"
      "Zygote\n"
      "Utility One\n"
      "Utility Two\n"
      "Tab One\n"
      "Tab Two: sharing process with Tab One\n"
      "Subframe: Tab One\n"
      "Subframe: Tab One (2)\n"
      "Subframe: Tab One (third)\n"
      "Subframe: Tab One (4)\n"
      "Extension Subframe: Tab One\n"
      "Extension Subframe: Tab Two\n"
      "Subframe: Tab Two\n"
      "Guest\n"
      "Worker\n",
      DumpSortedTasks());
}

TEST_F(TaskManagerImplTest, SortingCycles) {
  // Two tabs, with subframes in the other's process. This induces a cycle in
  // the TaskGroup dependencies, without being a cycle in the Tasks. This can
  // happen in practice.
  Task* tab1 = AddTask(200, Task::RENDERER, "Tab 1: Process 200", 10);
  AddTask(300, Task::RENDERER, "Subframe in Tab 1: Process 300", 10, tab1);
  Task* tab2 = AddTask(300, Task::RENDERER, "Tab 2: Process 300", 20);
  AddTask(200, Task::RENDERER, "Subframe in Tab 2: Process 200", 20, tab2);

  // Simulated GPU process.
  AddTask(100, Task::GPU, "Gpu Process", -1);

  // Two subframes that list each other as a parent (a cycle among groups).
  FakeTask* cycle1 = AddTask(501, Task::SANDBOX_HELPER, "Cycle 1", -1);
  FakeTask* cycle2 = AddTask(500, Task::ARC, "Cycle 2", -1, cycle1);
  cycle1->SetParent(cycle2);
  AddTask(501, Task::SANDBOX_HELPER, "Cycle 1a", -1, cycle2);

  // A cycle where both elements are in the same group.
  FakeTask* cycle3 = AddTask(600, Task::SANDBOX_HELPER, "Cycle 3", -1);
  FakeTask* cycle4 = AddTask(600, Task::ARC, "Cycle 4", -1, cycle3);
  cycle3->SetParent(cycle4);
  AddTask(600, Task::SANDBOX_HELPER, "Cycle 3a", -1, cycle4);

  // Tasks listing a cycle as their parent.
  AddTask(701, Task::EXTENSION, "Child of Cycle 3", -1, cycle3);
  AddTask(700, Task::PLUGIN, "Child of Cycle 4", -1, cycle4);

  // A task listing itself as parent.
  FakeTask* self_cycle = AddTask(800, Task::RENDERER, "Self Cycle", 5);
  self_cycle->SetParent(self_cycle);

  // Add a plugin child to tab1 and tab2.
  AddTask(900, Task::PLUGIN, "Plugin: Tab 2", 20, tab1);
  AddTask(901, Task::PLUGIN, "Plugin: Tab 1", 10, tab1);

  // Finish with a normal renderer task.
  AddTask(903, Task::RENDERER, "Tab: Normal Renderer", 30);

  // Cycles should wind up on the bottom of the list.
  EXPECT_EQ(
      "Browser\n"
      "Gpu Process\n"
      "Self Cycle\n"                      // RENDERER, tab id 5
      "Tab 1: Process 200\n"              // RENDERER, tab id 10
      "Subframe in Tab 2: Process 200\n"  // Same process as Tab 1.
      "Tab 2: Process 300\n"
      "Subframe in Tab 1: Process 300\n"  // Same process as Tab 2
      "Plugin: Tab 1\n"                   // PLUGIN child of Tab 1
      "Plugin: Tab 2\n"                   // PLUGIN child of Tab 2
      "Tab: Normal Renderer\n"            // RENDERER, tab id 30
      "Cycle 1\n"                         // SANBOX_HELPER
      "Cycle 1a\n"                        // Same process at 1.
      "Cycle 2\n"                         // Child of 1
      "Cycle 3\n"                         // Created after 1
      "Cycle 4\n"                         // Created after 3
      "Cycle 3a\n"                        // Created after 4
      "Child of Cycle 3\n"                // Child of 3
      "Child of Cycle 4\n",               // Child of 4
      DumpSortedTasks());
}

}  // namespace task_manager
