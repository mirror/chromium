// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef THIRD_PARTY_LEVELDATABASE_LEVELDB_CHROME_H_
#define THIRD_PARTY_LEVELDATABASE_LEVELDB_CHROME_H_

#include "base/files/file_path.h"
#include "leveldb/cache.h"
#include "leveldb/env.h"

namespace leveldb_chrome {

// Return the shared leveldb block cache for web APIs. The caller *does not*
// own the returned instance.
extern leveldb::Cache* GetSharedWebBlockCache();

// Return the shared leveldb block cache for browser (non web) APIs. The caller
// *does not* own the returned instance.
extern leveldb::Cache* GetSharedBrowserBlockCache();

// Determine if a leveldb::Env stores the file data in RAM.
extern bool IsMemEnv(const leveldb::Env* env);

// Creates an in-memory Env for which all files are stored in the heap.
extern leveldb::Env* NewMemEnv(leveldb::Env* base_env);

// Corrupt a closed database for testing purposes. After calling this function
// leveldb::OpenDB(...) will return a status where IsCorruption() returns true.
// Returns true if the database was successfully corrupted, false if not.
extern bool CorruptClosedDBForTesting(const base::FilePath& db_path);

}  // namespace leveldb_chrome

#endif  // THIRD_PARTY_LEVELDATABASE_LEVELDB_CHROME_H_
