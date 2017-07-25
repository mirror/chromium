// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/store/prefetch_store_utils.h"

#include <limits>

#include "base/rand_util.h"
#include "components/offline_pages/core/prefetch/prefetch_item.h"
#include "sql/connection.h"
#include "sql/statement.h"

namespace offline_pages {

int64_t GenerateOfflineId() {
  return base::RandGenerator(std::numeric_limits<int64_t>::max()) + 1;
}

bool DeletePrefetchItemByOfflineIdSync(sql::Connection* db,
                                       int64_t offline_id) {
  DCHECK(db);
  static const char kSql[] = "DELETE FROM prefetch_items WHERE offline_id=?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, offline_id);
  return statement.Run();
}

void LoadPrefetchItem(const sql::Statement& statement, PrefetchItem* item) {
  DCHECK_EQ(16, statement.ColumnCount());
  DCHECK(item);

  // Fields are assigned to the item in the order they are stored in the SQL
  // store (integer fields first).
  item->offline_id = statement.ColumnInt64(0);
  item->state = static_cast<PrefetchItemState>(statement.ColumnInt(1));
  item->generate_bundle_attempts = statement.ColumnInt(2);
  item->get_operation_attempts = statement.ColumnInt(3);
  item->download_operation_attempts = statement.ColumnInt(4);
  item->archive_body_length = statement.ColumnInt64(5);
  item->creation_time = base::Time() + base::TimeDelta::FromMicroseconds(
                                           statement.ColumnInt64(6));
  item->freshness_time = base::Time() + base::TimeDelta::FromMicroseconds(
                                            statement.ColumnInt64(7));
  item->error_code = static_cast<PrefetchItemErrorCode>(statement.ColumnInt(8));
  item->guid = statement.ColumnString(9);
  item->client_id.name_space = statement.ColumnString(10);
  item->client_id.id = statement.ColumnString(11);
  item->url = GURL(statement.ColumnString(12));
  item->final_archived_url = GURL(statement.ColumnString(13));
  item->operation_name = statement.ColumnString(14);
  item->archive_body_name = statement.ColumnString(15);
}

}  // namespace offline_pages
