// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/reading_list/reading_list_web_state_observer.h"

#import <Foundation/Foundation.h>

#include "base/memory/ptr_util.h"
#include "components/reading_list/ios/reading_list_model.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/reading_list/reading_list_entry_loading_util.h"
#include "ios/chrome/browser/reading_list/reading_list_model_factory.h"
#include "ios/web/public/navigation_item.h"
#include "ios/web/public/navigation_manager.h"
#include "ios/web/public/web_state/web_state_user_data.h"

#pragma mark - ReadingListWebStateObserverUserDataWrapper

namespace {
// The key under which ReadingListWebStateObserverUserDataWrapper are stored in
// a WebState's user data.
const void* const kObserverKey = &kObserverKey;
}  // namespace

// Wrapper class used to associated ReadingListWebStateObserver with their
// WebStates.
class ReadingListWebStateObserverUserDataWrapper
    : public base::SupportsUserData::Data {
 public:
  static ReadingListWebStateObserverUserDataWrapper* FromWebState(
      web::WebState* web_state,
      ReadingListModel* reading_list_model) {
    DCHECK(web_state);
    ReadingListWebStateObserverUserDataWrapper* wrapper =
        static_cast<ReadingListWebStateObserverUserDataWrapper*>(
            web_state->GetUserData(kObserverKey));
    if (!wrapper) {
      auto newDataWrapper =
          base::MakeUnique<ReadingListWebStateObserverUserDataWrapper>(
              web_state, reading_list_model);
      wrapper = newDataWrapper.get();
      web_state->SetUserData(kObserverKey, newDataWrapper.release());
    }
    return wrapper;
  }

  ReadingListWebStateObserverUserDataWrapper(
      web::WebState* web_state,
      ReadingListModel* reading_list_model)
      : observer_(web_state, reading_list_model) {
    DCHECK(web_state);
  }

  ReadingListWebStateObserver* observer() { return &observer_; }

 private:
  ReadingListWebStateObserver observer_;
};

#pragma mark - ReadingListWebStateObserver

ReadingListWebStateObserver* ReadingListWebStateObserver::FromWebState(
    web::WebState* web_state,
    ReadingListModel* reading_list_model) {
  return ReadingListWebStateObserverUserDataWrapper::FromWebState(
             web_state, reading_list_model)
      ->observer();
}

ReadingListWebStateObserver::~ReadingListWebStateObserver() {}

ReadingListWebStateObserver::ReadingListWebStateObserver(
    web::WebState* web_state,
    ReadingListModel* reading_list_model)
    : web::WebStateObserver(web_state),
      reading_list_model_(reading_list_model) {
  DCHECK(web_state);
  DCHECK(reading_list_model_);
}

void ReadingListWebStateObserver::DidStopLoading() {
  StopCheckingProgress();
}

void ReadingListWebStateObserver::PageLoaded(
    web::PageLoadCompletionStatus load_completion_status) {
  if (load_completion_status == web::PageLoadCompletionStatus::SUCCESS &&
      pending_url_.is_valid()) {
    reading_list_model_->SetReadStatus(pending_url_, true);
  }
  StopCheckingProgress();
}

void ReadingListWebStateObserver::WebStateDestroyed() {
  StopCheckingProgress();
  web_state()->RemoveUserData(kObserverKey);
}

void ReadingListWebStateObserver::StartCheckingProgress(
    const GURL& pending_url) {
  pending_url_ = pending_url;
  timer_.reset(new base::Timer(false, true));
  const base::TimeDelta kDelayUntilLoadingProgressIsChecked =
      base::TimeDelta::FromSeconds(1);
  timer_->Start(
      FROM_HERE, kDelayUntilLoadingProgressIsChecked,
      base::Bind(
          &ReadingListWebStateObserver::VerifyIfReadingListEntryStartedLoading,
          base::Unretained(this)));
}

void ReadingListWebStateObserver::StopCheckingProgress() {
  pending_url_ = GURL();
  timer_.reset();
}

void ReadingListWebStateObserver::VerifyIfReadingListEntryStartedLoading() {
  if (!pending_url_.is_valid()) {
    return;
  }
  const ReadingListEntry* entry =
      reading_list_model_->GetEntryByURL(pending_url_);
  if (!entry || entry->DistilledState() != ReadingListEntry::PROCESSED) {
    return;
  }
  double progress = web_state()->GetLoadingProgress();
  const double kMinimumExpectedProgress = 0.15;
  if (progress < kMinimumExpectedProgress) {
    reading_list::LoadReadingListDistilled(*entry, reading_list_model_,
                                           web_state());
  }
}
