// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sql/utils.h"

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/metrics/histogram.h"
#include "base/metrics/sparse_histogram.h"
#include "sql/recovery.h"
#include "third_party/sqlite/sqlite3.h"

namespace sql {
namespace utils {

namespace {
bool OpenAndRecoverDatabaseInternal(sql::Connection* db,
                                    const base::FilePath& db_path,
                                    bool recover_with_meta_version) {
  DCHECK(db);
  DCHECK(!db->is_open());
  DCHECK(!db->has_error_callback());
  const std::string histogram_tag =
      db->get_histogram_tag().empty() ? "" : ("." + db->get_histogram_tag());

  int db_error_primary = 0;
  auto error_callback_func = base::Bind(
      [](int* out_error, int error, sql::Statement*) {
        // Make primary error code from extended by & 0xFF.
        *out_error = error & 0xFF;
      },
      base::Unretained(&db_error_primary));
  db->set_error_callback(error_callback_func);

  bool db_is_open = db->Open(db_path);

  // Try to recover.
  if (db_is_open && db_error_primary == SQLITE_CORRUPT) {
    db->reset_error_callback();
    // May call LOG(FATAL) in some cases.
    if (recover_with_meta_version)
      sql::Recovery::RecoverDatabaseWithMetaVersion(db, db_path);
    else
      sql::Recovery::RecoverDatabase(db, db_path);
    db->Close();
    db->set_error_callback(error_callback_func);
    db_error_primary = 0;
    db_is_open = db->Open(db_path);

    // Add to histogram recovery result.
    {
      const std::string full_histogram_name =
          "Sqlite.SmartOpenRecovery" + histogram_tag;
      base::HistogramBase* histogram = base::BooleanHistogram::FactoryGet(
          full_histogram_name, base::HistogramBase::kUmaTargetedHistogramFlag);
      if (histogram)
        histogram->AddBoolean(db_error_primary == SQLITE_OK);
    }
  }

  if (db_error_primary == SQLITE_CORRUPT || db_error_primary == SQLITE_NOTADB ||
      (db_error_primary == SQLITE_CANTOPEN && base::DirectoryExists(db_path))) {
    if (db_is_open)
      db->Close();
    const bool delete_result = base::DeleteFile(db_path, true);

    // Add histogram to result of the file deletion.
    {
      const std::string full_histogram_name =
          "Sqlite.SmartOpenDeleteFile" + histogram_tag;
      base::HistogramBase* histogram = base::BooleanHistogram::FactoryGet(
          full_histogram_name, base::HistogramBase::kUmaTargetedHistogramFlag);
      if (histogram)
        histogram->AddBoolean(delete_result);
    }

    if (!delete_result)
      return false;
    db_error_primary = 0;
    db_is_open = db->Open(db_path);
  }

  db->reset_error_callback();
  if (!db_is_open || db_error_primary) {
    if (db_is_open)
      db->Close();
    LOG(ERROR) << histogram_tag << ". Unable to open database file.";
    {
      const std::string full_histogram_name =
          "Sqlite.SmartOpenError" + histogram_tag;
      base::HistogramBase* histogram = base::SparseHistogram::FactoryGet(
          full_histogram_name, base::HistogramBase::kUmaTargetedHistogramFlag);
      if (histogram)
        histogram->Add(db_error_primary);
    }
    return false;
  }
  return true;
}
}  // namespace

SQL_EXPORT bool OpenAndRecoverDatabase(sql::Connection* db,
                                       const base::FilePath& db_path) {
  return OpenAndRecoverDatabaseInternal(db, db_path, false);
}

SQL_EXPORT bool OpenAndRecoverDatabaseWithMetaVersion(
    sql::Connection* db,
    const base::FilePath& db_path) {
  return OpenAndRecoverDatabaseInternal(db, db_path, true);
}

}  // namespace utils
}  // namespace sql
