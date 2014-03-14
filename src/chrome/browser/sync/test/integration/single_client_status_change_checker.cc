// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/test/integration/single_client_status_change_checker.h"

#include "chrome/browser/sync/profile_sync_service.h"
#include "chrome/browser/sync/test/integration/profile_sync_service_harness.h"

SingleClientStatusChangeChecker::SingleClientStatusChangeChecker(
    ProfileSyncService* service) : service_(service) {}

SingleClientStatusChangeChecker::~SingleClientStatusChangeChecker() {}

void SingleClientStatusChangeChecker::InitObserver(
    ProfileSyncServiceHarness* obs) {
  service()->AddObserver(obs);
}

void SingleClientStatusChangeChecker::UninitObserver(
    ProfileSyncServiceHarness* obs) {
  service()->RemoveObserver(obs);
}
