// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/async_shutdown_task.h"

namespace chromeos {

namespace tether {

AsyncShutdownTask::AsyncShutdownTask() {}

AsyncShutdownTask::~AsyncShutdownTask() {}

void AsyncShutdownTask::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void AsyncShutdownTask::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

void AsyncShutdownTask::NotifyAsyncShutdownComplete() {
  for (auto& observer : observer_list_)
    observer.OnAsyncShutdownComplete();
}

}  // namespace tether

}  // namespace chromeos
