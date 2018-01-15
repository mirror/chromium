// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CRAZY_LINKER_RDEBUG_TASK_LIST_H
#define CRAZY_LINKER_RDEBUG_TASK_LIST_H

#include "crazy_linker_util.h"

namespace crazy {

class RDebug;
class SharedLibrary;

// Models a list of pending modifications to the global |_r_debug| list of
// shared variables visible to the debugger and Breakpad. This is used to
// implement deferred tasks that modify the global process state.
class LibraryTaskList {
 public:
  // Type of a notification function that takes a single opaque parameter.
  using NotifyFunc = void(void*);

  // Constructor takes a |delegate| parameter.
  explicit LibraryTaskList() = default;

  // Returns true iff the list of pending tasks is empty.
  bool IsEmpty() const { return list_.IsEmpty(); }

  // Returns true iff the list is active, i.e. there is a non-nullptr
  // notification callback registered.
  bool IsActive() const { return notify_func_ != nullptr; }

  // Add a new task for the addition of |library| to the global |_r_debug|
  // list. Note that |lib| cannot be nullptr.
  void AddRDebug(SharedLibrary* library);

  // Add a new task for the deletion of |library| to the list, then deletion
  // of |library| (which should simply un-map it from memory).
  //
  // Note that if a previous call to AddRDebug() with the same argument was
  // made before ApplyTasks() was called, this removes the task from the list
  // entirely, since there is no point in adding + removing the same
  // entry during Apply().
  void DelRDebug(SharedLibrary* library);

  // Reset the notification callback to a new function |notify_func| that
  // takes a single |notify_opaque| parameter. NOTE: A nullptr for
  // |notify_func| disables delayed tasks.
  void ResetCallback(NotifyFunc* notify_func, void* notify_opaque);

  // Call the notification callback if the task list is not empty.
  void NotifyIfPendingTasks() const;

  // Apply all modifications recorded in the list. |rdebug| must be a valid
  // RDebug instance. This also empties the list completely, unmapping any
  // pending removed libraries if needed.
  void ApplyPendingTasks(RDebug* rdebug);

 private:
  // Models a small deferred task. They correspond to the following
  // operations:
  //   ADD_ENTRY: A shared library has been loaded, add its link_map_t entry
  //              to the global |_r_debug| list.
  //
  //   DEL_ENTRY: A shared library has been unloaded, remove it from the global
  //              |_r_debug| list then unmap it from memory completely.
  struct Task {
    enum class Kind {
      ADD_ENTRY,
      DEL_ENTRY,
    };

    Task(Kind kind, SharedLibrary* library) : kind(kind), library(library) {}

    Kind kind = Kind::ADD_ENTRY;
    SharedLibrary* library = nullptr;  // only used for DEL_ENTRY.
  };

  NotifyFunc* notify_func_ = nullptr;
  void* notify_opaque_ = nullptr;
  Vector<Task> list_;
};

}  // namespace crazy

#endif  // CRAZY_LINKER_RDEBUG_TASK_LIST_H
