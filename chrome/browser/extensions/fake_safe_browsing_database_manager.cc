// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/fake_safe_browsing_database_manager.h"

#include <stddef.h>

#include <algorithm>
#include <iterator>
#include <vector>

#include "base/bind_helpers.h"
#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/safe_browsing/safe_browsing_service.h"
#include "components/safe_browsing/db/v4_protocol_manager_util.h"

namespace extensions {

FakeSafeBrowsingDatabaseManager::FakeSafeBrowsingDatabaseManager(bool enabled)
    : enabled_(enabled) {}

FakeSafeBrowsingDatabaseManager::~FakeSafeBrowsingDatabaseManager() {
}

void FakeSafeBrowsingDatabaseManager::Enable() {
  enabled_ = true;
}

void FakeSafeBrowsingDatabaseManager::Disable() {
  enabled_ = false;
}

void FakeSafeBrowsingDatabaseManager::ClearUnsafe() {
  unsafe_ids_.clear();
}

void FakeSafeBrowsingDatabaseManager::SetUnsafe(const std::string& a) {
  ClearUnsafe();
  unsafe_ids_.insert(a);
}

void FakeSafeBrowsingDatabaseManager::SetUnsafe(const std::string& a,
                                                const std::string& b) {
  SetUnsafe(a);
  unsafe_ids_.insert(b);
}

void FakeSafeBrowsingDatabaseManager::SetUnsafe(const std::string& a,
                                                const std::string& b,
                                                const std::string& c) {
  SetUnsafe(a, b);
  unsafe_ids_.insert(c);
}

void FakeSafeBrowsingDatabaseManager::SetUnsafe(const std::string& a,
                                                const std::string& b,
                                                const std::string& c,
                                                const std::string& d) {
  SetUnsafe(a, b, c);
  unsafe_ids_.insert(d);
}

void FakeSafeBrowsingDatabaseManager::AddUnsafe(const std::string& a) {
  unsafe_ids_.insert(a);
}

void FakeSafeBrowsingDatabaseManager::RemoveUnsafe(const std::string& a) {
  unsafe_ids_.erase(a);
}

bool FakeSafeBrowsingDatabaseManager::CheckExtensionIDs(
    const std::set<std::string>& extension_ids,
    Client* client) {
  if (!enabled_)
    return true;

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&safe_browsing::SafeBrowsingDatabaseManager::
                                    Client::OnCheckExtensionsResult,
                                base::Unretained(client), unsafe_ids_));
  return false;
}

}  // namespace extensions
