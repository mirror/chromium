// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/mock_password_store.h"

#include "base/memory/ptr_util.h"
#include "base/threading/sequenced_task_runner_handle.h"

namespace password_manager {

MockPasswordStore::MockPasswordStore() = default;
/*
bool MockPasswordStore::Init(
    const syncer::SyncableService::StartSyncFlare& flare,
    PrefService* prefs) {
  main_thread_runner_ = base::SequencedTaskRunnerHandle::Get();
  background_task_runner_ = main_thread_runner_;
  return true;
};*/

MockPasswordStore::~MockPasswordStore() = default;

scoped_refptr<base::SequencedTaskRunner>
MockPasswordStore::CreateBackgroundTaskRunner() const {
  return base::SequencedTaskRunnerHandle::Get();
}

void MockPasswordStore::InitOnBackgroundThread(
    const syncer::SyncableService::StartSyncFlare& flare) {}

}  // namespace password_manager
