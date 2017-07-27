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

void MakePrefetchItem(const sql::Statement& statement, PrefetchItem* item) {
  item->offline_id = statement.ColumnInt64(0);
  item->state = static_cast<PrefetchItemState>(statement.ColumnInt(1));
  item->request_archive_attempt_count = statement.ColumnInt(2);
  item->archive_body_length = statement.ColumnInt64(3);
  item->creation_time = base::Time::FromInternalValue(statement.ColumnInt64(4));
  item->freshness_time =
      base::Time::FromInternalValue(statement.ColumnInt64(5));
  item->error_code = static_cast<PrefetchItemErrorCode>(statement.ColumnInt(6));
  item->guid = statement.ColumnString(7);
  item->client_id =
      ClientId(statement.ColumnString(8), statement.ColumnString(9));
  item->url = GURL(statement.ColumnString(10));
  item->final_archived_url = GURL(statement.ColumnString(11));
  item->operation_name = statement.ColumnString(12);
  item->archive_body_name = statement.ColumnString(13);
  item->title = statement.ColumnString16(14);
  item->file_path = base::FilePath::FromUTF8Unsafe(statement.ColumnString(15));
  item->file_size = statement.ColumnInt64(16);
}

}  // namespace offline_pages
