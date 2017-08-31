// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/fake_async_shutdown_task.h"

namespace chromeos {

namespace tether {

FakeAsyncShutdownTask::FakeAsyncShutdownTask() {}

FakeAsyncShutdownTask::~FakeAsyncShutdownTask() {}

void FakeAsyncShutdownTask::NotifyAsyncShutdownComplete() {
  AsyncShutdownTask::NotifyAsyncShutdownComplete();
}

}  // namespace tether

}  // namespace chromeos
