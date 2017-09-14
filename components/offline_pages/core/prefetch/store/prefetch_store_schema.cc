// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/store/prefetch_store_schema.h"

#include <cstdio>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/stringprintf.h"
#include "base/task_runner_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/offline_store_types.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_utils.h"
#include "sql/connection.h"
#include "sql/meta_table.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {

// static
const int PrefetchStoreSchema::kCurrentVersion = 2;

// static
const int PrefetchStoreSchema::kCompatibleVersion = 1;

namespace {

// TODO(carlosk): propose improvements to MetaTable so that getters and setters
// of version information allow the caller to know about errors. Then remove
// these internal values from here.

// From MetaTable internals.
const char kVersionKey[] = "version";
const char kCompatibleVersionKey[] = "last_compatible_version";

const int kVersionError = -1;

bool SetVersionNumber(sql::MetaTable* meta_table, int version) {
  return meta_table->SetValue(kVersionKey, version);
}

bool SetCompatibleVersionNumber(sql::MetaTable* meta_table, int version) {
  return meta_table->SetValue(kCompatibleVersionKey, version);
}

int GetVersionNumber(sql::MetaTable* meta_table) {
  int version;
  if (meta_table->GetValue(kVersionKey, &version))
    return version;
  return kVersionError;
}

int GetCompatibleVersionNumber(sql::MetaTable* meta_table) {
  int version;
  if (meta_table->GetValue(kCompatibleVersionKey, &version))
    return version;
  return kVersionError;
}

// Name of the table with prefetch items.
const char kItemsTableName[] = "prefetch_items";
const char kItemsTempTableName[] = "prefetch_items_temp";
const char kQuotaTableName[] = "prefetch_downloader_quota";
const char kQuotaTempTableName[] = "prefetch_downloader_quota_temp";

// IMPORTANT #1: when making changes to these columns please also reflect them
// into:
// - PrefetchItem: update existing fields and all method implementations
//   (operator=, operator<<, ToString, etc).
// - PrefetchItemTest, PrefetchStoreTestUtil: update test related code to cover
//   the changed set of columns and PrefetchItem members.
// - MockPrefetchItemGenerator: so that its generated items consider all fields.
// IMPORTANT #2: the ordering of column types is important in SQLite 3 tables to
// simplify data retrieval. Columns with fixed length types must come first and
// variable length types must come later.
static const char kItemsTableCreationSql[] =
    "CREATE TABLE %s "
    // Fixed length columns come first.
    "(offline_id INTEGER PRIMARY KEY NOT NULL,"
    " state INTEGER NOT NULL DEFAULT 0,"
    " generate_bundle_attempts INTEGER NOT NULL DEFAULT 0,"
    " get_operation_attempts INTEGER NOT NULL DEFAULT 0,"
    " download_initiation_attempts INTEGER NOT NULL DEFAULT 0,"
    " archive_body_length INTEGER_NOT_NULL DEFAULT -1,"
    " creation_time INTEGER NOT NULL,"
    " freshness_time INTEGER NOT NULL,"
    " error_code INTEGER NOT NULL DEFAULT 0,"
    " file_size INTEGER NOT NULL DEFAULT -1,"
    // Variable length columns come later.
    " guid VARCHAR NOT NULL DEFAULT '',"
    " client_namespace VARCHAR NOT NULL DEFAULT '',"
    " client_id VARCHAR NOT NULL DEFAULT '',"
    " requested_url VARCHAR NOT NULL DEFAULT '',"
    " final_archived_url VARCHAR NOT NULL DEFAULT '',"
    " operation_name VARCHAR NOT NULL DEFAULT '',"
    " archive_body_name VARCHAR NOT NULL DEFAULT '',"
    " title VARCHAR NOT NULL DEFAULT '',"
    " file_path VARCHAR NOT NULL DEFAULT ''"
    ")";

bool CreatePrefetchItemsTable(sql::Connection* db, bool temporary) {
  const char* table_name = temporary ? kItemsTempTableName : kItemsTableName;
  if (db->DoesTableExist(table_name))
    return temporary ? false : true;  // Existing temp table is an error.
  sql::Statement statement(db->GetUniqueStatement(
      base::StringPrintf(kItemsTableCreationSql, table_name).c_str()));
  return statement.Run();
}

static const char kQuotaTableCreationSql[] =
    "CREATE TABLE %s "
    "(quota_id INTEGER PRIMARY KEY NOT NULL DEFAULT 1,"
    " update_time INTEGER NOT NULL,"
    " available_quota INTEGER NOT NULL DEFAULT 0)";

bool CreatePrefetchQuotaTable(sql::Connection* db, bool temporary) {
  const char* table_name = temporary ? kQuotaTempTableName : kQuotaTableName;
  if (db->DoesTableExist(table_name))
    return temporary ? false : true;  // Existing temp table is an error.
  sql::Statement statement(db->GetUniqueStatement(
      base::StringPrintf(kQuotaTableCreationSql, table_name).c_str()));
  return statement.Run();
}

bool CreateLatestSchema(sql::Connection* db) {
  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return false;

  if (!CreatePrefetchItemsTable(db, false) ||
      !CreatePrefetchQuotaTable(db, false)) {
    return false;
  }

  // This would be a great place to add indices when we need them.
  return transaction.Commit();
}

bool DropTable(sql::Connection* db, const char* table_name) {
  std::string sqlWithTableNames =
      base::StringPrintf("DROP TABLE %s", table_name);
  return db->Execute(sqlWithTableNames.c_str());
}

bool RenameTable(sql::Connection* db,
                 const char* from_table_name,
                 const char* to_table_name) {
  std::string sqlWithTableNames = base::StringPrintf(
      "ALTER TABLE %s RENAME TO %s", from_table_name, to_table_name);
  return db->Execute(sqlWithTableNames.c_str());
}

}  // namespace

PrefetchStoreSchema::PrefetchStoreSchema(sql::Connection* db) : db_(db){};

PrefetchStoreSchema::~PrefetchStoreSchema() = default;

bool PrefetchStoreSchema::CreateOrUpgradeIfNeeded() {
  DCHECK_GE(kCurrentVersion, kCompatibleVersion);
  sql::MetaTable meta_table;
  if (!meta_table.Init(db_, kCurrentVersion, kCompatibleVersion) ||
      !CreateLatestSchema(db_)) {
    return false;
  }

  const int compatible_version = GetCompatibleVersionNumber(&meta_table);
  const int current_version = GetVersionNumber(&meta_table);
  if (current_version == kVersionError || compatible_version == kVersionError)
    return false;
  DCHECK_GE(current_version, compatible_version);

  // Stored database version is newer and incompatible with what the current
  // running code supports (Chrome was downgraded).
  if (compatible_version > kCurrentVersion)
    return false;

  // Database is already at the latest version.
  if (current_version == kCurrentVersion)
    return true;

  // Schema upgrade code starts here.
  // Note: A series of if-else blocks was chosen to allow for more flexibility
  // in the upgrade logic than a single switch-case block would.

  if (current_version <= 1) {
    sql::Transaction transaction(db_);
    if (!transaction.Begin())
      return false;

    if (!CreatePrefetchItemsTable(db_, true))
      return false;

    const char kMigrateItemsSql[] =
        "INSERT INTO %s "
        " (offline_id, state, generate_bundle_attempts, get_operation_attempts,"
        "  download_initiation_attempts, archive_body_length, creation_time,"
        "  freshness_time, error_code, file_size, guid, client_namespace,"
        "  client_id, requested_url, final_archived_url, operation_name,"
        "  archive_body_name, title, file_path)"
        " SELECT "
        "  offline_id, state, generate_bundle_attempts, get_operation_attempts,"
        "  download_initiation_attempts, archive_body_length, creation_time,"
        "  freshness_time, error_code, file_size, guid, client_namespace,"
        "  client_id, requested_url, final_archived_url, operation_name,"
        "  archive_body_name, title, file_path"
        " FROM %s";
    sql::Statement copyStatement(db_->GetUniqueStatement(
        base::StringPrintf(kMigrateItemsSql, kItemsTempTableName,
                           kItemsTableName)
            .c_str()));
    if (!copyStatement.Run())
      return false;

    if (!DropTable(db_, kItemsTableName) ||
        !RenameTable(db_, kItemsTempTableName, kItemsTableName) ||
        !SetVersionNumber(&meta_table, kCurrentVersion) ||
        !SetCompatibleVersionNumber(&meta_table, kCompatibleVersion)) {
      return false;
    }

    if (!transaction.Commit())
      return false;
  }

  return true;
}

// static
const char* PrefetchStoreSchema::GetItemTableCreationSqlForTesting() {
  return kItemsTableCreationSql;
}

}  // namespace offline_pages
