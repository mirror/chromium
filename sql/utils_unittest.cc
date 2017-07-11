// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sql/utils.h"

#include <memory>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_piece.h"
#include "base/test/histogram_tester.h"
#include "sql/connection.h"
#include "sql/meta_table.h"
#include "sql/test/test_helpers.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/sqlite/sqlite3.h"

namespace sql {
namespace {

class SqlUtilsTest : public testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    file_ = temp_dir_.GetPath().AppendASCII("TestSqlUtilsSafeOpen");
    db_ = base::MakeUnique<Connection>();
    db_->set_histogram_tag("Test");
    ASSERT_TRUE(db_->Open(file_));
    ASSERT_TRUE(FillDatabase(&db()));
    db_->Close();
  }

  void TearDown() override {}

  Connection& db() { return *db_; }

  bool FillDatabase(Connection* db) {
    if (!db->Execute("CREATE TABLE x (t TEXT)"))
      return false;
    if (!db->Execute("INSERT INTO x VALUES ('This is a test')"))
      return false;
    if (!db->Execute("INSERT INTO x VALUES ('That was a test')"))
      return false;

    return true;
  }

 protected:
  base::ScopedTempDir temp_dir_;
  base::FilePath file_;
  std::unique_ptr<Connection> db_;
};

int64_t TruncateFile(const base::FilePath& path, double factor) {
  base::File f(path, base::File::FLAG_OPEN | base::File::FLAG_WRITE);
  if (!f.IsValid())
    return 0;
  f.SetLength(f.GetLength() * factor);
  return f.GetLength();
}

TEST_F(SqlUtilsTest, OpenAndRecoverDatabaseOnCorrrectFile) {
  base::HistogramTester histogram_tester;

  EXPECT_TRUE(utils::OpenAndRecoverDatabase(&db(), file_));
  size_t count;
  EXPECT_TRUE(test::CountTableRows(&db(), "x", &count));
  EXPECT_EQ(2U, count);

  histogram_tester.ExpectTotalCount("Sqlite.SmartOpenRecovery.Test", 0);
  histogram_tester.ExpectTotalCount("Sqlite.SmartOpenDeleteFile.Test", 0);
}

TEST_F(SqlUtilsTest, OpenAndRecoverDatabaseOnCorruptSizeFile) {
  base::HistogramTester histogram_tester;

  logging::ScopedLogAssertHandler s(
      base::Bind([](const char* file, int line, const base::StringPiece message,
                    const base::StringPiece stack_trace) {}));

  ASSERT_TRUE(test::CorruptSizeInHeader(file_));
  EXPECT_TRUE(utils::OpenAndRecoverDatabase(&db(), file_));
  size_t count;
  EXPECT_TRUE(test::CountTableRows(&db(), "x", &count));
  EXPECT_EQ(2U, count);

  histogram_tester.ExpectUniqueSample("Sqlite.SmartOpenRecovery.Test", 1, 1);
  histogram_tester.ExpectTotalCount("Sqlite.SmartOpenDeleteFile.Test", 0);
}

TEST_F(SqlUtilsTest, OpenAndRecoverDatabaseWithMetaOnCorruptSizeFile) {
  base::HistogramTester histogram_tester;

  ASSERT_TRUE(db_->Open(file_));
  MetaTable meta_table;
  ASSERT_TRUE(meta_table.Init(&db(), 1, 1));
  db_->Close();

  ASSERT_TRUE(test::CorruptSizeInHeader(file_));
  EXPECT_TRUE(utils::OpenAndRecoverDatabaseWithMetaVersion(&db(), file_));
  size_t count;
  EXPECT_TRUE(test::CountTableRows(&db(), "x", &count));
  EXPECT_EQ(2U, count);

  histogram_tester.ExpectUniqueSample("Sqlite.SmartOpenRecovery.Test", 1, 1);
  histogram_tester.ExpectTotalCount("Sqlite.SmartOpenDeleteFile.Test", 0);
}

TEST_F(SqlUtilsTest, OpenAndRecoverDatabaseOnTruncatedFile) {
  base::HistogramTester histogram_tester;

  // Corrupt databasefile
  ASSERT_GT(TruncateFile(file_, 0.5), 0);

  EXPECT_TRUE(utils::OpenAndRecoverDatabase(&db(), file_));
  size_t count;
  EXPECT_TRUE(test::CountTableRows(&db(), "x", &count));
  EXPECT_EQ(0U, count);

  histogram_tester.ExpectUniqueSample("Sqlite.SmartOpenRecovery.Test", 1, 1);
  histogram_tester.ExpectTotalCount("Sqlite.SmartOpenDeleteFile.Test", 0);
}

TEST_F(SqlUtilsTest, OpenAndRecoverDatabaseOnCorruptFileToMinimumSize) {
  base::HistogramTester histogram_tester;

  // Recovery method in OpenAndRecoverDatabaseWithMetaVersion may call
  // LOG(FATAL) in some cases of corrupted database, therefore to diasble
  // default handler.
  logging::ScopedLogAssertHandler s(
      base::Bind([](const char* file, int line, const base::StringPiece message,
                    const base::StringPiece stack_trace) {}));

  auto corrupt_file = file_.InsertBeforeExtensionASCII("_corrupt");
  ASSERT_TRUE(base::CopyFile(file_, corrupt_file));
  int64_t file_size;

  do {
    db().Close();

    // Corrupt databasefile
    file_size = TruncateFile(corrupt_file, 0.75);
    ASSERT_GT(file_size, 0);
    ASSERT_TRUE(base::CopyFile(corrupt_file, file_));

    // Try to open corrupted database
    EXPECT_TRUE(utils::OpenAndRecoverDatabase(&db(), file_));
  } while (file_size > 5);
}

TEST_F(SqlUtilsTest, OpenAndRecoverDatabaseOnFileNotADB) {
  base::HistogramTester histogram_tester;

  // Corrupt databasefile
  {
    base::File f(file_, base::File::FLAG_OPEN | base::File::FLAG_WRITE);
    ASSERT_TRUE(f.IsValid());
    int64_t length = f.GetLength();
    for (int64_t i = 0; i < length; i++) {
      ASSERT_EQ(1, f.WriteAtCurrentPos("\1", 1));
    }
    f.Close();
  }

  EXPECT_TRUE(utils::OpenAndRecoverDatabaseWithMetaVersion(&db(), file_));
  EXPECT_FALSE(db().DoesTableExist("x"));

  histogram_tester.ExpectTotalCount("Sqlite.SmartOpenRecovery.Test", 0);
  histogram_tester.ExpectUniqueSample("Sqlite.SmartOpenDeleteFile.Test", 1, 1);
}

TEST_F(SqlUtilsTest, OpenAndRecoverDatabaseOnFileIsDirectory) {
  base::HistogramTester histogram_tester;

  // Corrupt databasefile
  {
    ASSERT_TRUE(base::DeleteFile(file_, true));
    ASSERT_TRUE(base::CreateDirectory(file_));
  }

  EXPECT_TRUE(utils::OpenAndRecoverDatabaseWithMetaVersion(&db(), file_));
  EXPECT_FALSE(db().DoesTableExist("x"));

  histogram_tester.ExpectTotalCount("Sqlite.SmartOpenRecovery.Test", 0);
  histogram_tester.ExpectUniqueSample("Sqlite.SmartOpenDeleteFile.Test", 1, 1);
}

#if defined(OS_POSIX)
TEST_F(SqlUtilsTest, OpenAndRecoverDatabaseRemoveFailed) {
  base::HistogramTester histogram_tester;

  ASSERT_TRUE(base::DeleteFile(file_, true));
  ASSERT_TRUE(base::CreateDirectory(file_));

  auto failed_dir = file_.AppendASCII("Failed");
  ASSERT_TRUE(base::CreateDirectory(failed_dir));

  int permissions;
  ASSERT_TRUE(GetPosixFilePermissions(file_, &permissions));
  permissions ^= base::FILE_PERMISSION_WRITE_BY_USER;
  ASSERT_TRUE(SetPosixFilePermissions(file_, permissions));

  EXPECT_FALSE(utils::OpenAndRecoverDatabase(&db(), failed_dir));

  permissions ^= base::FILE_PERMISSION_WRITE_BY_USER;
  ASSERT_TRUE(SetPosixFilePermissions(file_, permissions));

  histogram_tester.ExpectTotalCount("Sqlite.SmartOpenRecovery.Test", 0);
  histogram_tester.ExpectUniqueSample("Sqlite.SmartOpenDeleteFile.Test", 0, 1);
}
#endif  // OS_POSIX

}  // namespace
}  // namespace sql
