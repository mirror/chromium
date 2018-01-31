// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sql/sql_export.h"

namespace sql {

// Makes sure that sqlite3_initialize() is called.
//
// Users of the APIs exposed in //sql do not need to worry about SQLite
// initialization, because sql::Connection calls this function internally.
//
// The function is exposed for other components that use SQLite indirectly. For
// example, on Linux and ChromeOS, //crypto uses the NSS library, which
// indirectly uses SQLite. To avoid initialization races, //crypto calls this
// function before initializing NSS.
SQL_EXPORT void EnsureSqliteInitialized();

}  // namespace sql