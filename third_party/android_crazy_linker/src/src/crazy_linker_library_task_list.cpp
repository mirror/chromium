// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crazy_linker_library_task_list.h"

#include "crazy_linker_debug.h"
#include "crazy_linker_globals.h"
#include "crazy_linker_rdebug.h"
#include "crazy_linker_shared_library.h"

namespace crazy {

void LibraryTaskList::AddRDebug(SharedLibrary* library) {
  if (library) {
    LOG("Adding ADD task for library %p [%s]", library, library->full_path_);

    list_.PushBack(Task(Task::Kind::ADD_ENTRY, library));
  }
}

void LibraryTaskList::DelRDebug(SharedLibrary* library) {
  // First, if an ADD_ENTRY task was already recorded for the same
  // |entry| value, just remove it and delete the library immediately.
  for (int n = 0; n < list_.GetCount(); ++n) {
    if (list_[n].kind == Task::Kind::ADD_ENTRY && list_[n].library == library) {
      LOG("Removing ADD task for library %p [%s]", library,
          library->full_path_);
      list_.RemoveAt(n);
      delete library;
      return;
    }
  }
  // Otherwise, append a new DEL_ENTRY task.
  LOG("Adding DEL task for library %p [%s]", library, library->full_path_);

  list_.PushBack(Task(Task::Kind::DEL_ENTRY, library));
}

void LibraryTaskList::ResetCallback(NotifyFunc* notify_func,
                                    void* notify_opaque) {
  notify_func_ = notify_func;
  notify_opaque_ = notify_opaque;
}

void LibraryTaskList::NotifyIfPendingTasks() const {
  if (!list_.IsEmpty() && notify_func_ != nullptr) {
    LOG("Notifying client");
    NotifyFunc* func = notify_func_;
    void* opaque = notify_opaque_;
    // Ensure the global lock is released during the call.
    ScopedGlobalsUnlocker unlocker;
    (*func)(opaque);
  }
}

void LibraryTaskList::ApplyPendingTasks(RDebug* rdebug) {
  LOG("Called");
  while (list_.GetCount() > 0) {
    Task task = list_.PopFirst();
    switch (task.kind) {
      case Task::Kind::ADD_ENTRY:
        LOG("Applying ADD task for library %p [%s]", __PRETTY_FUNCTION__,
            task.library, task.library->full_path_);
        rdebug->AddEntry(&task.library->link_map_);
        break;
      case Task::Kind::DEL_ENTRY:
        LOG("Applying DEL task for library %p [%s]", __PRETTY_FUNCTION__,
            task.library, task.library->full_path_);
        rdebug->DelEntry(&task.library->link_map_);
        delete task.library;
        break;
    }
  }
}

}  // namespace crazy
