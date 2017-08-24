// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/metrics_finalization_task.h"

#include <memory>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "components/offline_pages/core/offline_time_utils.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {
namespace {

// In-memory representation of URL metadata fetched from SQLite storage.
struct PrefetchItemStats {
  PrefetchItemStats(int64_t offline_id,
                    int generate_bundle_attempts,
                    int get_operation_attempts,
                    int download_initiation_attempts,
                    int64_t archive_body_length,
                    base::Time creation_time,
                    PrefetchItemErrorCode error_code,
                    int64_t file_size)
      : offline_id(offline_id),
        generate_bundle_attempts(generate_bundle_attempts),
        get_operation_attempts(get_operation_attempts),
        download_initiation_attempts(download_initiation_attempts),
        archive_body_length(archive_body_length),
        creation_time(creation_time),
        error_code(error_code),
        file_size(file_size) {}

  int64_t offline_id;
  int generate_bundle_attempts;
  int get_operation_attempts;
  int download_initiation_attempts;
  int64_t archive_body_length;
  base::Time creation_time;
  PrefetchItemErrorCode error_code;
  int64_t file_size;
};

std::vector<PrefetchItemStats> FetchUrlsSync(sql::Connection* db) {
  static const char kSql[] = R"(
  SELECT offline_id, generate_bundle_attempts, get_operation_attempts,
    download_initiation_attempts, archive_body_length, creation_time,
    error_code, file_size
  FROM prefetch_items
  WHERE state = ?
)";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, static_cast<int>(PrefetchItemState::FINISHED));

  std::vector<PrefetchItemStats> urls;
  while (statement.Step()) {
    urls.emplace_back(
        statement.ColumnInt64(0),  // offline_id
        statement.ColumnInt(1),    // generate_bundle_attempts
        statement.ColumnInt(2),    // get_operation_attempts
        statement.ColumnInt(3),    // download_initiation_attempts
        statement.ColumnInt64(4),  // archive_body_length
        FromDatabaseTime(statement.ColumnInt64(5)),  // creation_time
        static_cast<PrefetchItemErrorCode>(
            statement.ColumnInt(6)),  // error_code
        statement.ColumnInt64(7)      // file_size
        );
  }

  return urls;
}

bool MarkUrlAsZombie(sql::Connection* db,
                     base::Time freshness_time,
                     int64_t offline_id) {
  static const char kSql[] =
      "UPDATE prefetch_items SET state = ?, freshness_time = ? WHERE "
      "offline_id = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt(0, static_cast<int>(PrefetchItemState::ZOMBIE));
  statement.BindInt(1, ToDatabaseTime(freshness_time));
  statement.BindInt64(2, offline_id);
  return statement.Run();
}

enum class FileSizePercentRange {
  LESS_THAN_50 = 49,
  FROM_50_TO_75 = 50,
  FROM_75_TO_90 = 75,
  FROM_90_TO_99 = 90,
  FROM_99_TO_100 = 99,
  FROM_100_TO_101 = 101,
  FROM_101_TO_110 = 110,
  FROM_110_TO_130 = 130,
  FROM_130_TO_200 = 200,
  MORE_THAN_200 = 201,
  MAX = MORE_THAN_200
};

FileSizePercentRange GetRangeForRatio(double ratio) {
  DCHECK_NE(1.0, ratio);
  // These ranges are closed on the lower limit and open on the upper one.
  if (ratio < .5)
    return FileSizePercentRange::LESS_THAN_50;
  if (ratio < .75)
    return FileSizePercentRange::FROM_50_TO_75;
  if (ratio < .9)
    return FileSizePercentRange::FROM_75_TO_90;
  if (ratio < .99)
    return FileSizePercentRange::FROM_90_TO_99;
  if (ratio < 1.0)
    return FileSizePercentRange::FROM_99_TO_100;
  // These ranges are open on the lower limit and closed on the upper one.
  if (ratio <= 1.01)
    return FileSizePercentRange::FROM_100_TO_101;
  if (ratio <= 1.1)
    return FileSizePercentRange::FROM_101_TO_110;
  if (ratio <= 1.3)
    return FileSizePercentRange::FROM_110_TO_130;
  if (ratio <= 2.0)
    return FileSizePercentRange::FROM_130_TO_200;

  return FileSizePercentRange::MORE_THAN_200;
}

void ReportMetricsFor(const PrefetchItemStats& url, const base::Time now) {
  // Lifetime reporting.
  static const int four_weeks_in_seconds = 60 * 60 * 24 * 28;
  const bool successful = url.error_code == PrefetchItemErrorCode::SUCCESS;
  base::TimeDelta lifetime = now - url.creation_time;
  UMA_HISTOGRAM_CUSTOM_COUNTS(
      (successful ? "OfflinePages.Prefetching.ItemLifetime.Successful"
                  : "OfflinePages.Prefetching.ItemLifetime.Failed"),
      lifetime.InSeconds(), 1, four_weeks_in_seconds, 50);

  // Error code reporting.
  UMA_HISTOGRAM_ENUMERATION("OfflinePages.Prefetching.FinishedItemErrorCode",
                            url.error_code, PrefetchItemErrorCode::MAX);

  // Unexpected file size reporting.
  if (successful && url.archive_body_length > 0 && url.file_size >= 0 &&
      url.archive_body_length != url.file_size) {
    FileSizePercentRange difference_range =
        GetRangeForRatio(double(url.file_size) / url.archive_body_length);
    UMA_HISTOGRAM_ENUMERATION(
        "OfflinePages.Prefetching.DownloadedArchiveSizeVsExpected",
        difference_range, FileSizePercentRange::MAX);
  }

  // Attempt counts reporting.
  static const int max_retries_ever = 20;
  UMA_HISTOGRAM_EXACT_LINEAR(
      "OfflinePages.Prefetching.ActionRetryAttempts.GeneratePageBundle",
      url.generate_bundle_attempts, max_retries_ever);
  UMA_HISTOGRAM_EXACT_LINEAR(
      "OfflinePages.Prefetching.ActionRetryAttempts.GetOperation",
      url.get_operation_attempts, max_retries_ever);
  UMA_HISTOGRAM_EXACT_LINEAR(
      "OfflinePages.Prefetching.ActionRetryAttempts.DownloadInitiation",
      url.download_initiation_attempts, max_retries_ever);
}

bool SelectUrlsToPrefetchSync(sql::Connection* db) {
  if (!db)
    return false;

  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return false;

  const std::vector<PrefetchItemStats> urls = FetchUrlsSync(db);

  base::Time now = base::Time::Now();
  for (const auto& url : urls) {
    MarkUrlAsZombie(db, now, url.offline_id);
  }

  if (transaction.Commit()) {
    for (const auto& url : urls) {
      DVLOG(1) << "Finalized prefetch item: (" << url.offline_id << ", "
               << url.generate_bundle_attempts << ", "
               << url.get_operation_attempts << ", "
               << url.download_initiation_attempts << ", "
               << url.archive_body_length << ", " << url.creation_time << ", "
               << static_cast<int>(url.error_code) << ", " << url.file_size
               << ")";
      ReportMetricsFor(url, now);
    }
    return true;
  }

  return false;
}

}  // namespace

MetricsFinalizationTask::MetricsFinalizationTask(PrefetchStore* prefetch_store)
    : prefetch_store_(prefetch_store), weak_factory_(this) {}

MetricsFinalizationTask::~MetricsFinalizationTask() {}

void MetricsFinalizationTask::Run() {
  prefetch_store_->Execute(
      base::BindOnce(&SelectUrlsToPrefetchSync),
      base::BindOnce(&MetricsFinalizationTask::MetricsFinalized,
                     weak_factory_.GetWeakPtr()));
}

void MetricsFinalizationTask::MetricsFinalized(bool result) {
  TaskComplete();
}

}  // namespace offline_pages
