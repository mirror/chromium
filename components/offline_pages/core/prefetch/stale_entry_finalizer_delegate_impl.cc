// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/stale_entry_finalizer_delegate_impl.h"

#include "base/logging.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"

namespace offline_pages {

const std::vector<PrefetchItemState>
StaleEntryFinalizerDelegateImpl::StatesToExpireForBucket(
    StaleEntryFinalizerTask::Bucket b) {
  static const std::vector<PrefetchItemState> kBucket1StatesToExpire{
      PrefetchItemState::NEW_REQUEST,
  };
  static const std::vector<PrefetchItemState> kBucket2StatesToExpire{
      PrefetchItemState::AWAITING_GCM, PrefetchItemState::RECEIVED_GCM,
      PrefetchItemState::RECEIVED_BUNDLE,
  };
  static const std::vector<PrefetchItemState> kBucket3StatesToExpire{
      PrefetchItemState::DOWNLOADING,
  };

  switch (b) {
    case StaleEntryFinalizerTask::BUCKET_1:
      return kBucket1StatesToExpire;

    case StaleEntryFinalizerTask::BUCKET_2:
      return kBucket2StatesToExpire;

    case StaleEntryFinalizerTask::BUCKET_3:
      return kBucket3StatesToExpire;
  }
  NOTREACHED();
  return std::vector<PrefetchItemState>();
}

const base::TimeDelta StaleEntryFinalizerDelegateImpl::FreshnessPeriodForBucket(
    StaleEntryFinalizerTask::Bucket b) {
  switch (b) {
    case StaleEntryFinalizerTask::BUCKET_1:
      return base::TimeDelta::FromDays(1);
    case StaleEntryFinalizerTask::BUCKET_2:
      return base::TimeDelta::FromDays(1);
    case StaleEntryFinalizerTask::BUCKET_3:
      return base::TimeDelta::FromDays(2);
  }
  NOTREACHED();
  return base::TimeDelta::FromDays(1);
}

PrefetchItemErrorCode StaleEntryFinalizerDelegateImpl::ErrorCodeForStaleEntry(
    PrefetchItemState state) {
  switch (state) {
    case PrefetchItemState::NEW_REQUEST:
      return PrefetchItemErrorCode::STALE_AT_NEW_REQUEST;
    case PrefetchItemState::AWAITING_GCM:
      return PrefetchItemErrorCode::STALE_AT_AWAITING_GCM;
    case PrefetchItemState::RECEIVED_GCM:
      return PrefetchItemErrorCode::STALE_AT_RECEIVED_GCM;
    case PrefetchItemState::RECEIVED_BUNDLE:
      return PrefetchItemErrorCode::STALE_AT_RECEIVED_BUNDLE;
    case PrefetchItemState::DOWNLOADING:
      return PrefetchItemErrorCode::STALE_AT_DOWNLOADING;
    default:
      NOTREACHED();
  }
  return PrefetchItemErrorCode::STALE_AT_UNKNOWN;
}

base::Time StaleEntryFinalizerDelegateImpl::Now() {
  return base::Time::Now();
}

}  // namespace offline_pages
