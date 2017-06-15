// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/store/prefetch_store_sql_utils.h"

#include "components/offline_pages/core/prefetch/prefetch_item.h"
#include "sql/connection.h"
#include "sql/statement.h"

namespace offline_pages {

void MakePrefetchItem(sql::Statement* statement, PrefetchItem* item) {
  DCHECK(statement);
  DCHECK(item);

  // Fields are assigned to the item in the order the are stored in the SQL
  // store (integer fields first).
  item->offline_id = statement->ColumnInt64(0);
  item->state = static_cast<PrefetchItemState>(statement->ColumnInt(1));
  item->request_archive_attempt_count = statement->ColumnInt(2);
  item->archive_body_length = statement->ColumnInt64(3);
  item->creation_time =
      base::Time::FromInternalValue(statement->ColumnInt64(4));
  item->freshness_time =
      base::Time::FromInternalValue(statement->ColumnInt64(5));
  item->error_code =
      static_cast<PrefetchItemErrorCode>(statement->ColumnInt(6));
  item->guid = statement->ColumnString(7);
  item->client_id.name_space = statement->ColumnString(8);
  item->client_id.id = statement->ColumnString(9);
  item->url = GURL(statement->ColumnString(10));
  item->final_archived_url = GURL(statement->ColumnString(11));
  item->operation_name = statement->ColumnString(12);
  item->archive_body_name = statement->ColumnString(13);
}

bool GetPrefetchItemByOfflineIdSync(sql::Connection* db,
                                    int64_t offline_id,
                                    PrefetchItem* item) {
  DCHECK(db);
  DCHECK(item);

  const char kSql[] = "SELECT * FROM prefetch_items WHERE offline_id = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, offline_id);

  if (statement.Step()) {
    MakePrefetchItem(&statement, item);
    return true;
  }

  return false;
}

bool DeletePrefetchItemByOfflineIdSync(sql::Connection* db,
                                       int64_t offline_id) {
  DCHECK(db);
  static const char kSql[] = "DELETE FROM prefetch_items WHERE offline_id=?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, offline_id);
  return statement.Run();
}

std::unique_ptr<StoreUpdateResult<PrefetchItem>> BuildStoreErrorResultForIds(
    StoreState store_state,
    const std::vector<int64_t>& offline_ids,
    ItemActionStatus action_status) {
  std::unique_ptr<StoreUpdateResult<PrefetchItem>> result(
      new StoreUpdateResult<PrefetchItem>(store_state));
  for (const auto& offline_id : offline_ids)
    result->item_statuses.push_back(std::make_pair(offline_id, action_status));
  return result;
}

}  // namespace offline_pages
