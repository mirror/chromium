// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/store/prefetch_downloader_quota.h"

#include "components/offline_pages/core/offline_time_utils.h"
#include "sql/connection.h"
#include "sql/statement.h"

namespace offline_pages {

namespace {
const int64_t kDailyQuota = 20LL * 1024 * 1024;  // 20 MB
const int64_t kMicrosecondsPerDay = 24LL * 60 * 60 * 1000 * 1000;
}  // namespace

// static
int64_t PrefetchDownloaderQuota::GetAvailableQuota(sql::Connection* db) {
  static const char kSql[] =
      "SELECT update_time, available_quota FROM prefetch_downloader_quota"
      " WHERE quota_id = 1";
  if (db == nullptr)
    return -1LL;

  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));

  if (!statement.Step()) {
    if (!statement.Succeeded())
      return -1LL;

    return kDailyQuota;
  }

  base::Time update_time = FromDatabaseTime(statement.ColumnInt64(0));
  int64_t available_quota = statement.ColumnInt64(1);

  int64_t remaining_quota =
      available_quota + (base::Time::Now() - update_time).InMicroseconds() *
                            kDailyQuota / kMicrosecondsPerDay;

  return remaining_quota > kDailyQuota ? kDailyQuota : remaining_quota;
}

// static
bool PrefetchDownloaderQuota::SetAvailableQuota(sql::Connection* db,
                                                int64_t quota) {
  static const char kSql[] =
      "INSERT OR REPLACE INTO prefetch_downloader_quota"
      " (quota_id, update_time, available_quota)"
      " VALUES (1, ?, ?)";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, ToDatabaseTime(base::Time::Now()));
  statement.BindInt64(1, quota);
  return statement.Run();
}
}  // namespace offline_pages
