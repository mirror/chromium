// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/clear_storage_task.h"

#include <algorithm>
#include <vector>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/memory/ptr_util.h"
#include "base/time/clock.h"
#include "base/time/default_clock.h"
#include "base/time/time.h"
#include "components/offline_pages/core/client_policy_controller.h"
#include "components/offline_pages/core/model/offline_store_utils.h"
#include "components/offline_pages/core/offline_page_client_policy.h"
#include "components/offline_pages/core/offline_page_metadata_store_sql.h"
#include "components/offline_pages/core/offline_time_utils.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {

using LifetimeType = LifetimePolicy::LifetimeType;
using ClearStorageResult = ClearStorageTask::ClearStorageResult;

namespace {

#define PAGE_INFO_PROJECTION                                               \
  " offline_id, client_namespace, client_id, file_size, last_access_time," \
  " file_path"

// This struct needs to be in sync with |PAGE_INFO_PROJECTION|.
struct PageInfo {
  PageInfo();
  PageInfo(const PageInfo& other);
  int64_t offline_id;
  ClientId client_id;
  int64_t file_size;
  base::Time last_access_time;
  base::FilePath file_path;
};

PageInfo::PageInfo() = default;
PageInfo::PageInfo(const PageInfo& other) = default;

// Maximum % of total available storage that will be occupied by offline pages
// before a storage clearup.
const double kOfflinePageStorageLimit = 0.3;
// The target % of storage usage we try to reach below when expiring pages.
const double kOfflinePageStorageClearThreshold = 0.1;
// The time that the storage cleanup will be triggered again since the last
// one.
const base::TimeDelta kClearStorageInterval = base::TimeDelta::FromMinutes(10);

// Make sure this is in sync with |PAGE_INFO_PROJECTION| above.
PageInfo MakePageInfo(sql::Statement* statement) {
  PageInfo page_info;
  page_info.offline_id = statement->ColumnInt64(0);
  page_info.client_id =
      ClientId(statement->ColumnString(1), statement->ColumnString(2));
  page_info.file_size = statement->ColumnInt64(3);
  page_info.last_access_time = FromDatabaseTime(statement->ColumnInt64(4));
  page_info.file_path =
      base::FilePath::FromUTF8Unsafe(statement->ColumnString(5));
  return page_info;
}

bool ShouldClearPages(const base::Time& last_start_time,
                      const base::Time& start_time,
                      const ArchiveManager::StorageStats& storage_stats) {
  int64_t total_size = storage_stats.total_archives_size;
  int64_t free_space = storage_stats.free_disk_space;
  if (total_size == 0)
    return false;

  // If the size of all offline pages is more than limit, or it's larger than a
  // specified percentage of all available storage space on the disk we'll clear
  // offline pages until we reach below the limit.
  if (total_size >= (total_size + free_space) * kOfflinePageStorageLimit)
    return true;
  // If it's been more than the pre-defined interval since the last time we
  // clear the storage, we should clear pages.
  if (last_start_time == base::Time() ||
      start_time - last_start_time >= kClearStorageInterval) {
    return true;
  }
  // Otherwise there's no need to clear storage right now.
  return false;
}

bool IsExpired(ClientPolicyController* policy_controller,
               const PageInfo& page_info,
               const base::Time& start_time) {
  const LifetimePolicy& policy =
      policy_controller->GetPolicy(page_info.client_id.name_space)
          .lifetime_policy;
  return start_time - page_info.last_access_time >= policy.expiration_period;
}

std::unique_ptr<std::vector<PageInfo>> GetAllTemporaryPageInfos(
    ClientPolicyController* policy_controller,
    sql::Connection* db) {
  auto result = base::MakeUnique<std::vector<PageInfo>>();

  std::vector<std::string> temp_namespaces =
      policy_controller->GetNamespacesRemovedOnCacheReset();

  const char kSql[] = "SELECT " PAGE_INFO_PROJECTION
                      " FROM offlinepages_v1"
                      " WHERE client_namespace = ?";
  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return result;

  for (const auto& name_space : temp_namespaces) {
    sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
    statement.BindString(0, name_space);
    while (statement.Step()) {
      result->emplace_back(MakePageInfo(&statement));
    }
  }

  if (!transaction.Commit())
    result->clear();
  return result;
}

std::unique_ptr<std::vector<PageInfo>> GetPageInfosToClear(
    ClientPolicyController* policy_controller,
    const base::Time& start_time,
    const ArchiveManager::StorageStats& stats,
    sql::Connection* db) {
  std::map<std::string, int> namespace_page_count;
  auto page_infos_to_delete = base::MakeUnique<std::vector<PageInfo>>();
  std::vector<PageInfo> pages_remaining;
  int64_t remaining_size = 0;

  // Gets all temporary pages and sort by last accessed time.
  auto page_infos = GetAllTemporaryPageInfos(policy_controller, db);
  std::sort(page_infos->begin(), page_infos->end(),
            [](const PageInfo& a, const PageInfo& b) -> bool {
              return a.last_access_time > b.last_access_time;
            });

  for (const auto& page_info : *page_infos) {
    // If the page is expired, put it in the list to delete later.
    if (IsExpired(policy_controller, page_info, start_time)) {
      page_infos_to_delete->push_back(page_info);
      continue;
    }

    // If the namespace of the page already has more pages than limit, this page
    // needs to be deleted.
    std::string name_space = page_info.client_id.name_space;
    int page_limit =
        policy_controller->GetPolicy(name_space).lifetime_policy.page_limit;
    if (page_limit != kUnlimitedPages &&
        namespace_page_count[name_space] >= page_limit) {
      page_infos_to_delete->push_back(page_info);
      continue;
    }

    // Otherwise the page needs to be kept, in case the storage usage is still
    // higher than the threshold, and we need to clear more pages.
    pages_remaining.push_back(page_info);
    remaining_size += page_info.file_size;
  }

  // If storage usage is already below the threshold (assuming page deletion
  // will always succeed), there's no need to clear more pages.
  // Otherwise, pages needs to be cleared from oldest to newest, until storage
  // usage is below the threshold.
  int64_t space_to_release =
      remaining_size - (stats.total_archives_size + stats.free_disk_space) *
                           kOfflinePageStorageClearThreshold;
  for (auto page_info = pages_remaining.rbegin();
       page_info != pages_remaining.rend() && space_to_release > 0;
       ++page_info) {
    page_infos_to_delete->push_back(*page_info);
    space_to_release -= page_info->file_size;
  }

  return page_infos_to_delete;
}

bool DeleteArchiveSync(const base::FilePath& file_path) {
  // Delete the file only, |false| for recursive.
  return base::DeleteFile(file_path, false);
}

// Deletes a page from the store by |offline_id|.
bool DeletePageEntryByOfflineIdSync(sql::Connection* db, int64_t offline_id) {
  static const char kSql[] = "DELETE FROM offlinepages_v1 WHERE offline_id = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, offline_id);
  return statement.Run();
}

std::pair<size_t, DeletePageResult> ClearPages(
    ClientPolicyController* policy_controller,
    const base::Time& start_time,
    const ArchiveManager::StorageStats& stats,
    sql::Connection* db) {
  DeletePageResult result = DeletePageResult::SUCCESS;
  size_t pages_cleared = 0;

  auto page_infos =
      GetPageInfosToClear(policy_controller, start_time, stats, db);

  for (const auto& page_info : *page_infos) {
    if (!base::PathExists(page_info.file_path) ||
        DeleteArchiveSync(page_info.file_path)) {
      if (DeletePageEntryByOfflineIdSync(db, page_info.offline_id)) {
        pages_cleared++;
      }
    }
  }
  if (pages_cleared != page_infos->size())
    result = DeletePageResult::STORE_FAILURE;
  return std::make_pair(pages_cleared, result);
}

}  // namespace

ClearStorageTask::ClearStorageTask(OfflinePageMetadataStoreSQL* store,
                                   ArchiveManager* archive_manager,
                                   ClientPolicyController* policy_controller,
                                   const base::Time& last_start_time,
                                   ClearStorageCallback callback)
    : store_(store),
      archive_manager_(archive_manager),
      policy_controller_(policy_controller),
      callback_(std::move(callback)),
      last_start_time_(last_start_time),
      clock_(new base::DefaultClock()),
      weak_ptr_factory_(this) {}

ClearStorageTask::~ClearStorageTask() {}

void ClearStorageTask::Run() {
  start_time_ = clock_->Now();
  archive_manager_->GetStorageStats(
      base::Bind(&ClearStorageTask::OnGetStorageStatsDone,
                 weak_ptr_factory_.GetWeakPtr()));
}

void ClearStorageTask::SetClockForTesting(std::unique_ptr<base::Clock> clock) {
  clock_ = std::move(clock);
}

void ClearStorageTask::OnGetStorageStatsDone(
    const ArchiveManager::StorageStats& stats) {
  if (!ShouldClearPages(last_start_time_, start_time_, stats)) {
    InformClearStorageDone(0, ClearStorageResult::UNNECESSARY);
    return;
  }
  store_->Execute(
      base::BindOnce(&ClearPages, policy_controller_, start_time_, stats),
      base::BindOnce(&ClearStorageTask::OnClearPagesDone,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ClearStorageTask::OnClearPagesDone(
    std::pair<size_t, DeletePageResult> result) {
  ClearStorageResult clear_result = ClearStorageResult::SUCCESS;
  if (result.second != DeletePageResult::SUCCESS)
    clear_result = ClearStorageResult::DELETE_FAILURE;
  InformClearStorageDone(result.first, clear_result);
}

void ClearStorageTask::InformClearStorageDone(size_t pages_cleared,
                                              ClearStorageResult result) {
  std::move(callback_).Run(start_time_, pages_cleared, result);
  TaskComplete();
}

}  // namespace offline_pages
