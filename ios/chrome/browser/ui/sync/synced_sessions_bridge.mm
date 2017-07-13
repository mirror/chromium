// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/sync/synced_sessions_bridge.h"

#include "components/browser_sync/profile_sync_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/sync/driver/sync_service.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/signin/signin_manager_factory.h"
#include "ios/chrome/browser/sync/ios_chrome_profile_sync_service_factory.h"
#include "ios/chrome/browser/sync/sync_setup_service.h"
#include "ios/chrome/browser/sync/sync_setup_service_factory.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace synced_sessions {

#pragma mark - SyncedSessionsObserverBridge

SyncedSessionsObserverBridge::SyncedSessionsObserverBridge(
    id<SyncedSessionsObserver> owner,
    ios::ChromeBrowserState* browserState)
    : SyncObserverBridge(
          owner,
          IOSChromeProfileSyncServiceFactory::GetForBrowserState(browserState)),
      owner_(owner),
      signin_manager_(
          ios::SigninManagerFactory::GetForBrowserState(browserState)),
      sync_service_(
          IOSChromeProfileSyncServiceFactory::GetForBrowserState(browserState)),
      browser_state_(browserState),
      signin_manager_observer_(this),
      first_sync_cycle_is_completed_(false) {
  signin_manager_observer_.Add(signin_manager_);
}

SyncedSessionsObserverBridge::~SyncedSessionsObserverBridge() {}

#pragma mark - SyncObserverBridge

void SyncedSessionsObserverBridge::OnStateChanged(syncer::SyncService* sync) {
  if (!signin_manager_->IsAuthenticated())
    first_sync_cycle_is_completed_ = false;
  [owner_ onSyncStateChanged];
}

void SyncedSessionsObserverBridge::OnSyncCycleCompleted(
    syncer::SyncService* sync) {
  if (sync_service_->GetActiveDataTypes().Has(syncer::SESSIONS))
    first_sync_cycle_is_completed_ = true;
  [owner_ onSyncStateChanged];
}

void SyncedSessionsObserverBridge::OnSyncConfigurationCompleted(
    syncer::SyncService* sync) {
  [owner_ reloadSessions];
}

void SyncedSessionsObserverBridge::OnForeignSessionUpdated(
    syncer::SyncService* sync) {
  [owner_ reloadSessions];
}

bool SyncedSessionsObserverBridge::IsFirstSyncCycleCompleted() {
  return first_sync_cycle_is_completed_;
}

#pragma mark - SigninManagerBase::Observer

void SyncedSessionsObserverBridge::GoogleSignedOut(
    const std::string& account_id,
    const std::string& username) {
  first_sync_cycle_is_completed_ = false;
  [owner_ reloadSessions];
}

#pragma mark - Signin and syncing status

bool SyncedSessionsObserverBridge::IsSignedIn() {
  SigninManager* signin_manager =
      ios::SigninManagerFactory::GetForBrowserState(browser_state_);
  return signin_manager->IsAuthenticated();
}

bool SyncedSessionsObserverBridge::IsSyncing() {
  if (!IsSignedIn())
    return false;

  SyncSetupService* syncSetupService =
      SyncSetupServiceFactory::GetForBrowserState(browser_state_);

  bool syncEnabled = syncSetupService->IsSyncEnabled();
  bool noSyncError = (syncSetupService->GetSyncServiceState() ==
                      SyncSetupService::kNoSyncServiceError);

  return syncEnabled && noSyncError && !IsFirstSyncCycleCompleted();
}

}  // namespace synced_sessions
