// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <limits.h>
#include <stdint.h>

#include "base/timer/timer.h"
#include "chrome/browser/browsing_data/history_counter.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/history/web_history_service_factory.h"
#include "chrome/common/pref_names.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/web_history_service.h"
#include "content/public/browser/browser_thread.h"

namespace {
static const int64_t kWebHistoryTimeoutSeconds = 10;
}

HistoryCounter::HistoryCounter() : pref_name_(prefs::kDeleteBrowsingHistory),
                                   has_synced_visits_(false),
                                   local_counting_finished_(false),
                                   web_counting_finished_(false),
                                   testing_web_history_service_(nullptr) {
}

HistoryCounter::~HistoryCounter() {
}

const std::string& HistoryCounter::GetPrefName() const {
  return pref_name_;
}

bool HistoryCounter::HasTrackedTasks() {
  return cancelable_task_tracker_.HasTrackedTasks();
}

void HistoryCounter::SetWebHistoryServiceForTesting(
    history::WebHistoryService* service) {
  testing_web_history_service_ = service;
}

void HistoryCounter::Count() {
  // Reset the state.
  cancelable_task_tracker_.TryCancelAll();
  web_history_request_.reset();
  has_synced_visits_ = false;

  // Count the locally stored items.
  local_counting_finished_ = false;

  history::HistoryService* service =
      HistoryServiceFactory::GetForProfileWithoutCreating(GetProfile());

  service->GetHistoryCount(
      GetPeriodStart(),
      base::Time::Max(),
      base::Bind(&HistoryCounter::OnGetLocalHistoryCount,
                 base::Unretained(this)),
      &cancelable_task_tracker_);

  // If the history sync is enabled, test if there is at least one synced item.
  // If the testing web history service is present, use that one instead.
  history::WebHistoryService* web_history = testing_web_history_service_
      ? testing_web_history_service_
      : WebHistoryServiceFactory::GetForProfile(GetProfile());

  if (!web_history) {
    web_counting_finished_ = true;
    return;
  }

  web_counting_finished_ = false;

  web_history_timeout_.Start(
      FROM_HERE,
      base::TimeDelta::FromSeconds(kWebHistoryTimeoutSeconds),
      this,
      &HistoryCounter::OnWebHistoryTimeout);

  history::QueryOptions options;
  options.max_count = 1;
  options.begin_time = GetPeriodStart();
  options.end_time = base::Time::Max();
  web_history_request_ = web_history->QueryHistory(
      base::string16(),
      options,
      base::Bind(&HistoryCounter::OnGetWebHistoryCount,
                 base::Unretained(this)));

  // TODO(msramek): Include web history count when there is an API for it.
}

void HistoryCounter::OnGetLocalHistoryCount(
    history::HistoryCountResult result) {
  // Ensure that all callbacks are on the same thread, so that we do not need
  // a mutex for |MergeResults|.
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!result.success) {
    LOG(ERROR) << "Failed to count the local history.";
    return;
  }

  local_result_ = result.count;
  local_counting_finished_ = true;
  MergeResults();
}

void HistoryCounter::OnGetWebHistoryCount(
    history::WebHistoryService::Request* request,
    const base::DictionaryValue* result) {
  // Ensure that all callbacks are on the same thread, so that we do not need
  // a mutex for |MergeResults|.
  DCHECK(thread_checker_.CalledOnValidThread());

  // If the timeout for this request already fired, ignore the result.
  if (!web_history_timeout_.IsRunning())
    return;

  web_history_timeout_.Stop();

  // If the query failed, err on the safe side and inform the user that they
  // may have history items stored in Sync. Otherwise, we expect at least one
  // entry in the "event" list.
  const base::ListValue* events;
  has_synced_visits_ =
      !result ||
      (result->GetList("event", &events) && !events->empty());
  web_counting_finished_ = true;
  MergeResults();
}

void HistoryCounter::OnWebHistoryTimeout() {
  // Ensure that all callbacks are on the same thread, so that we do not need
  // a mutex for |MergeResults|.
  DCHECK(thread_checker_.CalledOnValidThread());

  // If the query timed out, err on the safe side and inform the user that they
  // may have history items stored in Sync.
  web_history_request_.reset();
  has_synced_visits_ = true;
  web_counting_finished_ = true;
  MergeResults();
}

void HistoryCounter::MergeResults() {
  if (!local_counting_finished_ || !web_counting_finished_)
    return;

  ReportResult(make_scoped_ptr(new HistoryResult(
      this, local_result_, has_synced_visits_)));
}

HistoryCounter::HistoryResult::HistoryResult(
    const HistoryCounter* source,
    ResultInt value,
    bool has_synced_visits)
    : FinishedResult(source, value),
      has_synced_visits_(has_synced_visits) {
}

HistoryCounter::HistoryResult::~HistoryResult() {
}
