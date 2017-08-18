// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/history/ios_browsing_history_driver.h"

#include <utility>

#include "base/mac/bind_objc_block.h"
#include "base/strings/utf_string_conversions.h"
#include "components/browsing_data/core/history_notice_utils.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/history/history_utils.h"
#include "ios/chrome/browser/history/web_history_service_factory.h"
#include "ios/chrome/browser/ui/history/ios_browsing_history_driver_delegate.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#pragma mark - IOSBrowsingHistoryDriver

IOSBrowsingHistoryDriver::IOSBrowsingHistoryDriver(
    ios::ChromeBrowserState* browser_state,
    id<IOSBrowsingHistoryDriverDelegate> delegate)
    : browser_state_(browser_state), delegate_(delegate) {}

IOSBrowsingHistoryDriver::~IOSBrowsingHistoryDriver() {
  delegate_ = nil;
}

#pragma mark - Private methods

void IOSBrowsingHistoryDriver::OnQueryComplete(
    const std::vector<BrowsingHistoryService::HistoryEntry>& results,
    const BrowsingHistoryService::QueryResultsInfo& query_results_info) {
  if ([delegate_
          respondsToSelector:@selector
          (IOSBrowsingHistoryDriverOnQueryComplete:results:query_results_info
                                                     :)]) {
    [delegate_ IOSBrowsingHistoryDriverOnQueryComplete:this
                                               results:results
                                    query_results_info:query_results_info];
  }
}

void IOSBrowsingHistoryDriver::OnRemoveVisitsComplete() {
  // Ignored.
}

void IOSBrowsingHistoryDriver::OnRemoveVisitsFailed() {
  // Ignored.
}

void IOSBrowsingHistoryDriver::OnRemoveVisits(
    const std::vector<history::ExpireHistoryArgs>& expire_list) {
  // Ignored.
}

void IOSBrowsingHistoryDriver::HistoryDeleted() {
  if ([delegate_ respondsToSelector:@selector
                 (IOSBrowsingHistoryDriverDidObserveHistoryDeletion:)]) {
    [delegate_ IOSBrowsingHistoryDriverDidObserveHistoryDeletion:this];
  }
}

void IOSBrowsingHistoryDriver::HasOtherFormsOfBrowsingHistory(
    bool has_other_forms,
    bool has_synced_results) {
  if ([delegate_ respondsToSelector:@selector
                 (IOSBrowsingHistoryDriver:
                     shouldShowNoticeAboutOtherFormsOfBrowsingHistory:)]) {
    [delegate_ IOSBrowsingHistoryDriver:this
        shouldShowNoticeAboutOtherFormsOfBrowsingHistory:has_other_forms];
  }
}

bool IOSBrowsingHistoryDriver::AllowHistoryDeletions() {
  // It is unclear if this behavior was purposeful or accidental. When
  // refactoring iOS to use BrowsingHistoryService the original behavior was
  // kept, which was never gate HistoryDeletions based on a policy. The policy
  // may not currently ever be set on iOS, so this may be okay.
  return true;
}

bool IOSBrowsingHistoryDriver::ShouldHideWebHistoryUrl(const GURL& url) {
  return !ios::CanAddURLToHistory(url);
}

history::WebHistoryService* IOSBrowsingHistoryDriver::GetWebHistoryService() {
  return ios::WebHistoryServiceFactory::GetForBrowserState(browser_state_);
}

void IOSBrowsingHistoryDriver::ShouldShowNoticeAboutOtherFormsOfBrowsingHistory(
    const syncer::SyncService* sync_service,
    history::WebHistoryService* history_service,
    base::Callback<void(bool)> callback) {
  browsing_data::ShouldShowNoticeAboutOtherFormsOfBrowsingHistory(
      sync_service, history_service, std::move(callback));
}
