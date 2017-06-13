// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SQL_UTILS_H_
#define SQL_UTILS_H_

#include <string>

#include "base/files/file_path.h"
#include "sql/connection.h"

namespace sql {
namespace utils {

// Opens |db| connections. Connection must be no set error-handling callback.
// Tries to recover the database if the file is corrupted. Creates a new
// database if the recovery failed.
SQL_EXPORT bool OpenAndRecoverDatabase(sql::Connection* db,
                                       const base::FilePath& db_path);

// Variant of SmartOpenDatabase () which requires that the database has a
// valid meta table with a version value. If this information cannot be
// determined, the database is considered as unrecoverable.
SQL_EXPORT bool OpenAndRecoverDatabaseWithMetaVersion(
    sql::Connection* db,
    const base::FilePath& db_path);

}  // namespace utils
}  // namespace sql

#endif  // SQL_UTILS_H_
