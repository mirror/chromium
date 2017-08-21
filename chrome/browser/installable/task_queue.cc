// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/installable/task_queue.h"

TaskQueue::TaskQueue() {}
TaskQueue::~TaskQueue() {}

void TaskQueue::Insert(Task task) {
  tasks_.push_back(task);
}

void TaskQueue::Reset() {
  tasks_.clear();
  paused_tasks_.clear();
}

bool TaskQueue::HasPaused() const {
  return !paused_tasks_.empty();
}

void TaskQueue::UnpauseAll() {
  for (const auto& task : paused_tasks_)
    Insert(task);

  paused_tasks_.clear();
}

Task& TaskQueue::Current() {
  DCHECK(!tasks_.empty());
  return tasks_[0];
}

void TaskQueue::PauseCurrent() {
  paused_tasks_.push_back(Current());
  Next();
}

void TaskQueue::Next() {
  DCHECK(!tasks_.empty());
  tasks_.erase(tasks_.begin());
}

bool TaskQueue::IsEmpty() const {
  // TODO(mcgreevy): try to remove this method by removing the need to
  // explicitly call WorkOnTask.
  return tasks_.empty();
}
