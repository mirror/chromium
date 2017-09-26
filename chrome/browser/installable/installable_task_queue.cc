// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/installable/installable_task_queue.h"

InstallableTaskQueue::InstallableTaskQueue() {}
InstallableTaskQueue::~InstallableTaskQueue() {}

// static
bool InstallableTaskQueue::IsParamsForPwaCheck(
    const InstallableParams& params) {
  return params.check_installable && params.fetch_valid_primary_icon;
}

void InstallableTaskQueue::Add(InstallableTask task) {
  tasks_.push_back(task);
}

void InstallableTaskQueue::PauseCurrent() {
  paused_tasks_.push_back(Current());
  Next();
}

void InstallableTaskQueue::UnpauseAll() {
  for (const auto& task : paused_tasks_)
    Add(task);

  paused_tasks_.clear();
}

bool InstallableTaskQueue::HasCurrent() const {
  return !tasks_.empty();
}

bool InstallableTaskQueue::HasPaused() const {
  return !paused_tasks_.empty();
}

bool InstallableTaskQueue::HasPwaCheck() const {
  for (const auto& task : tasks_) {
    if (IsParamsForPwaCheck(task.first))
      return true;
  }

  for (const auto& task : paused_tasks_) {
    if (IsParamsForPwaCheck(task.first))
      return true;
  }

  return false;
}

InstallableTask& InstallableTaskQueue::Current() {
  DCHECK(!tasks_.empty());
  return tasks_[0];
}

void InstallableTaskQueue::Next() {
  DCHECK(!tasks_.empty());
  tasks_.erase(tasks_.begin());
}

void InstallableTaskQueue::Reset() {
  tasks_.clear();
  paused_tasks_.clear();
}
