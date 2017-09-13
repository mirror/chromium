// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/store/prefetch_store_schema.h"

#include <limits>

#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/offline_time_utils.h"
#include "components/offline_pages/core/prefetch/mock_prefetch_item_generator.h"
#include "components/offline_pages/core/prefetch/prefetch_item.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_test_util.h"
#include "sql/connection.h"
#include "sql/meta_table.h"
#include "sql/statement.h"
#include "sql/transaction.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

static const char kSomeTableCreationSql[] =
    "CREATE TABLE some_table "
    "(id INTEGER PRIMARY KEY NOT NULL,"
    " value INTEGER NOT NULL)";

static const char kAnotherTableCreationSql[] =
    "CREATE TABLE another_table "
    "(id INTEGER PRIMARY KEY NOT NULL,"
    " name VARCHAR NOT NULL)";

TEST(PrefetchStoreSchemaPreconditionTest,
     TestSqliteCreateTableIsTransactional) {
  sql::Connection db;
  ASSERT_TRUE(db.OpenInMemory());

  sql::Transaction transaction(&db);
  ASSERT_TRUE(transaction.Begin());
  EXPECT_TRUE(db.Execute(kSomeTableCreationSql));
  EXPECT_TRUE(db.Execute(kAnotherTableCreationSql));
  transaction.Rollback();

  EXPECT_FALSE(db.DoesTableExist("some_table"));
  EXPECT_FALSE(db.DoesTableExist("another_table"));
}

TEST(PrefetchStoreSchemaPreconditionTest, TestSqliteDropTableIsTransactional) {
  sql::Connection db;
  ASSERT_TRUE(db.OpenInMemory());
  EXPECT_TRUE(db.Execute(kSomeTableCreationSql));
  EXPECT_TRUE(db.Execute(kAnotherTableCreationSql));

  sql::Transaction transaction(&db);
  ASSERT_TRUE(transaction.Begin());
  EXPECT_TRUE(db.Execute("DROP TABLE some_table"));
  EXPECT_TRUE(db.Execute("DROP TABLE another_table"));
  transaction.Rollback();

  EXPECT_TRUE(db.DoesTableExist("some_table"));
  EXPECT_TRUE(db.DoesTableExist("another_table"));
}

TEST(PrefetchStoreSchemaPreconditionTest, TestSqliteAlterTableIsTransactional) {
  sql::Connection db;
  ASSERT_TRUE(db.OpenInMemory());
  EXPECT_TRUE(db.Execute(kSomeTableCreationSql));

  sql::Transaction transaction(&db);
  ASSERT_TRUE(transaction.Begin());
  EXPECT_TRUE(db.Execute("ALTER TABLE some_table ADD new_column VARCHAR NULL"));
  EXPECT_TRUE(db.Execute("ALTER TABLE some_table RENAME TO another_table"));
  transaction.Rollback();

  EXPECT_TRUE(db.DoesTableExist("some_table"));
  EXPECT_FALSE(db.DoesColumnExist("some_table", "new_column"));
  EXPECT_FALSE(db.DoesTableExist("another_table"));
}

class PrefetchStoreSchemaTest : public testing::Test {
 public:
  PrefetchStoreSchemaTest() = default;
  ~PrefetchStoreSchemaTest() override = default;

  void SetUp() override {
    db_ = base::MakeUnique<sql::Connection>();
    ASSERT_TRUE(db_->OpenInMemory());
    schema_ = base::MakeUnique<PrefetchStoreSchema>(db_.get());
    ASSERT_FALSE(sql::MetaTable::DoesTableExist(db_.get()));
  }

 protected:
  std::unique_ptr<sql::Connection> db_;
  std::unique_ptr<PrefetchStoreSchema> schema_;
};

TEST_F(PrefetchStoreSchemaTest, TestSchemaCreationFromNothing) {
  EXPECT_TRUE(schema_->CreateOrUpgradeIfNeeded());
  EXPECT_TRUE(db_->DoesTableExist("prefetch_items"));
  EXPECT_TRUE(db_->DoesTableExist("prefetch_downloader_quota"));
  sql::MetaTable meta_table;
  EXPECT_TRUE(meta_table.Init(db_.get(), std::numeric_limits<int>::max(),
                              std::numeric_limits<int>::max()));
  EXPECT_EQ(PrefetchStoreSchema::kCurrentVersion,
            meta_table.GetVersionNumber());
  EXPECT_EQ(PrefetchStoreSchema::kCompatibleVersion,
            meta_table.GetCompatibleVersionNumber());
}

TEST_F(PrefetchStoreSchemaTest,
       TestMissingTablesAreCreatedEvenAtLatestVersion) {
  sql::MetaTable meta_table;
  EXPECT_TRUE(meta_table.Init(db_.get(), PrefetchStoreSchema::kCurrentVersion,
                              PrefetchStoreSchema::kCompatibleVersion));
  EXPECT_EQ(PrefetchStoreSchema::kCurrentVersion,
            meta_table.GetVersionNumber());
  EXPECT_EQ(PrefetchStoreSchema::kCompatibleVersion,
            meta_table.GetCompatibleVersionNumber());

  EXPECT_TRUE(schema_->CreateOrUpgradeIfNeeded());
  EXPECT_TRUE(db_->DoesTableExist("prefetch_items"));
  EXPECT_TRUE(db_->DoesTableExist("prefetch_downloader_quota"));
}

TEST_F(PrefetchStoreSchemaTest, TestOnlyMissingTablesAreCreated) {
  EXPECT_TRUE(schema_->CreateOrUpgradeIfNeeded());
  EXPECT_TRUE(db_->DoesTableExist("prefetch_items"));
  EXPECT_TRUE(db_->DoesTableExist("prefetch_downloader_quota"));

  EXPECT_TRUE(db_->Execute("DROP TABLE prefetch_items"));
  EXPECT_TRUE(schema_->CreateOrUpgradeIfNeeded());
  EXPECT_TRUE(db_->DoesTableExist("prefetch_items"));
  EXPECT_TRUE(db_->DoesTableExist("prefetch_downloader_quota"));

  EXPECT_TRUE(db_->Execute("DROP TABLE prefetch_downloader_quota"));
  EXPECT_TRUE(schema_->CreateOrUpgradeIfNeeded());
  EXPECT_TRUE(db_->DoesTableExist("prefetch_items"));
  EXPECT_TRUE(db_->DoesTableExist("prefetch_downloader_quota"));
}

const char kV0ItemsTableCreationSql[] =
    "CREATE TABLE prefetch_items"
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
    " file_size INTEGER NOT NULL DEFAULT 0,"
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

const char kV0QuotaTableCreationSql[] =
    "CREATE TABLE prefetch_downloader_quota"
    "(quota_id INTEGER PRIMARY KEY NOT NULL DEFAULT 1,"
    " update_time INTEGER NOT NULL,"
    " available_quota INTEGER NOT NULL DEFAULT 0)";

const char kV0ItemInsertSql[] =
    "INSERT INTO prefetch_items"
    " (offline_id, state, generate_bundle_attempts, get_operation_attempts,"
    "  download_initiation_attempts, archive_body_length, creation_time,"
    "  freshness_time, error_code, file_size, guid, client_namespace,"
    "  client_id, requested_url, final_archived_url, operation_name,"
    "  archive_body_name, title, file_path)"
    " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

const char kV0ItemSelectSql[] =
    "SELECT "
    " offline_id, state, generate_bundle_attempts, get_operation_attempts,"
    "  download_initiation_attempts, archive_body_length, creation_time,"
    "  freshness_time, error_code, file_size, guid, client_namespace,"
    "  client_id, requested_url, final_archived_url, operation_name,"
    "  archive_body_name, title, file_path"
    " FROM prefetch_items";

const char kV0QuotaInsertSql[] =
    "INSERT INTO prefetch_downloader_quota"
    " (quota_id, update_time, available_quota)"
    " VALUES (?, ?, ?)";

const char kV0QuotaSelectSql[] =
    "SELECT quota_id, update_time, available_quota"
    " FROM prefetch_downloader_quota";

// Tests that a migration from the initially deployed version of the schema,
// as it was for chromium/src at 90113a2c01ca9ff77042daacd8282a4c16aade85, is
// correctly migrated to the final, current version without losing data.
TEST_F(PrefetchStoreSchemaTest, TestMigrationFromV0) {
  sql::MetaTable meta_table;
  EXPECT_TRUE(meta_table.Init(db_.get(), 1, 1));
  EXPECT_EQ(1, meta_table.GetVersionNumber());
  EXPECT_EQ(1, meta_table.GetCompatibleVersionNumber());
  EXPECT_TRUE(db_->Execute(kV0ItemsTableCreationSql));
  EXPECT_TRUE(db_->Execute(kV0QuotaTableCreationSql));

  sql::Statement insertStatement1(db_->GetUniqueStatement(kV0ItemInsertSql));
  for (int i = 0; i <= 9; ++i)
    insertStatement1.BindInt(i, i + 1);
  // insertStatement1.BindInt(0, 1);
  // insertStatement1.BindInt(1, 2);
  // insertStatement1.BindInt(2, 3);
  // insertStatement1.BindInt(3, 4);
  // insertStatement1.BindInt(4, 5);
  // insertStatement1.BindInt(5, 6);
  // insertStatement1.BindInt(6, 7);
  // insertStatement1.BindInt(7, 8);
  // insertStatement1.BindInt(8, 9);
  // insertStatement1.BindInt(9, 10);

  for (int i = 10; i <= 18; ++i)
    insertStatement1.BindString(i, std::string(1, 'a' + i - 10));
  // insertStatement1.BindString(10, "a");
  // insertStatement1.BindString(11, "b");
  // insertStatement1.BindString(12, "c");
  // insertStatement1.BindString(13, "d");
  // insertStatement1.BindString(14, "e");
  // insertStatement1.BindString(15, "f");
  // insertStatement1.BindString(16, "g");
  // insertStatement1.BindString(17, "h");
  // insertStatement1.BindString(18, "i");
  EXPECT_TRUE(insertStatement1.Run());

  sql::Statement insertStatement2(db_->GetUniqueStatement(kV0QuotaInsertSql));
  insertStatement2.BindInt(0, 1);
  insertStatement2.BindInt(1, 2);
  insertStatement2.BindInt(2, 3);
  EXPECT_TRUE(insertStatement2.Run());

  EXPECT_TRUE(schema_->CreateOrUpgradeIfNeeded());

  sql::Statement selectStatement1(db_->GetUniqueStatement(kV0ItemSelectSql));
  ASSERT_TRUE(selectStatement1.Step());
  for (int i = 0; i <= 9; ++i)
    EXPECT_EQ(i + 1, selectStatement1.ColumnInt(i))
        << "Wrong value at column " << i;
  // EXPECT_EQ(1, selectStatement1.ColumnInt(0));
  // EXPECT_EQ(2, selectStatement1.ColumnInt(1));
  // EXPECT_EQ(3, selectStatement1.ColumnInt(2));
  // EXPECT_EQ(4, selectStatement1.ColumnInt(3));
  // EXPECT_EQ(5, selectStatement1.ColumnInt(4));
  // EXPECT_EQ(6, selectStatement1.ColumnInt(5));
  // EXPECT_EQ(7, selectStatement1.ColumnInt(6));
  // EXPECT_EQ(8, selectStatement1.ColumnInt(7));
  // EXPECT_EQ(9, selectStatement1.ColumnInt(8));
  // EXPECT_EQ(10, selectStatement1.ColumnInt(9));
  for (int i = 10; i <= 18; ++i)
    EXPECT_EQ(std::string(1, 'a' + i - 10), selectStatement1.ColumnString(i));
  // EXPECT_EQ("a", selectStatement1.ColumnString(10));
  // EXPECT_EQ("b", selectStatement1.ColumnString(11));
  // EXPECT_EQ("c", selectStatement1.ColumnString(12));
  // EXPECT_EQ("d", selectStatement1.ColumnString(13));
  // EXPECT_EQ("e", selectStatement1.ColumnString(14));
  // EXPECT_EQ("f", selectStatement1.ColumnString(15));
  // EXPECT_EQ("g", selectStatement1.ColumnString(16));
  // EXPECT_EQ("h", selectStatement1.ColumnString(17));
  // EXPECT_EQ("i", selectStatement1.ColumnString(18));
  EXPECT_FALSE(selectStatement1.Step());

  sql::Statement selectStatement2(db_->GetUniqueStatement(kV0QuotaSelectSql));
  ASSERT_TRUE(selectStatement2.Step());
  EXPECT_EQ(1, selectStatement2.ColumnInt(0));
  EXPECT_EQ(2, selectStatement2.ColumnInt(1));
  EXPECT_EQ(3, selectStatement2.ColumnInt(2));
  EXPECT_FALSE(selectStatement2.Step());
}

}  // namespace offline_pages
