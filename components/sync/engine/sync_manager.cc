// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine/sync_manager.h"

namespace syncer {

SyncCredentials::SyncCredentials() = default;

SyncCredentials::SyncCredentials(const SyncCredentials& other) = default;

SyncCredentials::~SyncCredentials() = default;

SyncManager::ChangeDelegate::~ChangeDelegate() = default;

SyncManager::ChangeObserver::~ChangeObserver() = default;

SyncManager::Observer::~Observer() = default;

SyncManager::InitArgs::InitArgs()
    : enable_local_sync_backend(false),
      extensions_activity(nullptr),
      change_delegate(nullptr),
      encryptor(nullptr),
      cancelation_signal(nullptr) {}

SyncManager::InitArgs::~InitArgs() = default;

SyncManager::SyncManager() = default;

SyncManager::~SyncManager() = default;

}  // namespace syncer
