// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/store/prefetch_downloader_quota.h"

#include "base/time/clock.h"
#include "base/time/time.h"
#include "components/offline_pages/core/offline_time_utils.h"
#include "sql/connection.h"
#include "sql/statement.h"

namespace offline_pages {

// static
const int64_t PrefetchDownloaderQuota::kMaxDailyQuota =
    20LL * 1024 * 1024;  // 20 MB

namespace {
const int64_t kMicrosecondsPerDay = 24LL * 60 * 60 * 1000 * 1000;

// Normalize quota to [0, kMaxDailyQuota].
int64_t NormalizeQuota(int64_t quota) {
  if (quota < 0)
    return 0;
  if (quota > PrefetchDownloaderQuota::kMaxDailyQuota)
    return PrefetchDownloaderQuota::kMaxDailyQuota;
  return quota;
}

}  // namespace

PrefetchDownloaderQuota::PrefetchDownloaderQuota(sql::Connection* db,
                                                 base::Clock* clock)
    : db_(db), clock_(clock) {
  DCHECK(db_);
  DCHECK(clock_);
}

PrefetchDownloaderQuota::~PrefetchDownloaderQuota() = default;

int64_t PrefetchDownloaderQuota::GetAvailableQuota() {
  static const char kSql[] =
      "SELECT update_time, available_quota FROM prefetch_downloader_quota"
      " WHERE quota_id = 1";
  if (db_ == nullptr || clock_ == nullptr)
    return -1LL;

  sql::Statement statement(db_->GetCachedStatement(SQL_FROM_HERE, kSql));

  if (!statement.Step()) {
    if (!statement.Succeeded())
      return -1LL;

    return kMaxDailyQuota;
  }

  base::Time update_time = FromDatabaseTime(statement.ColumnInt64(0));
  int64_t available_quota = statement.ColumnInt64(1);

  int64_t remaining_quota =
      available_quota + ((clock_->Now() - update_time).InMicroseconds()) *
                            kMaxDailyQuota / kMicrosecondsPerDay;

  return NormalizeQuota(remaining_quota);
}

bool PrefetchDownloaderQuota::SetAvailableQuota(int64_t quota) {
  static const char kSql[] =
      "INSERT OR REPLACE INTO prefetch_downloader_quota"
      " (quota_id, update_time, available_quota)"
      " VALUES (1, ?, ?)";
  if (db_ == nullptr || clock_ == nullptr)
    return false;

  quota = NormalizeQuota(quota);

  sql::Statement statement(db_->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, ToDatabaseTime(clock_->Now()));
  statement.BindInt64(1, quota);
  return statement.Run();
}
}  // namespace offline_pages
