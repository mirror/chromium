// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef THIRD_PARTY_LEVELDATABASE_LEVELDB_CHROME_H_
#define THIRD_PARTY_LEVELDATABASE_LEVELDB_CHROME_H_

#include "base/callback.h"
#include "base/containers/linked_list.h"
#include "base/gtest_prod_util.h"
#include "leveldb/db.h"
#include "leveldb/options.h"
#include "third_party/leveldatabase/leveldb_exports.h"

namespace base {
namespace trace_event {
class MemoryAllocatorDump;
class ProcessMemoryDump;
}  // namespace trace_event
}  // namespace base

namespace leveldb {
class Cache;
}  // namespace leveldb

namespace leveldb_chrome {

// Create the default leveldb options object suitable for leveldb operations.
struct LEVELDB_EXPORT Options : public leveldb::Options {
  Options();
};

// Tracks databases open via OpenDatabase() method and exposes them to
// memory-infra. The class is thread safe.
class LEVELDB_EXPORT DBTracker {
 public:
  // DBTracker singleton instance.
  static DBTracker* GetInstance();

  // Returns the memory-infra dump for |tracked_db|. Can be used to attach
  // additional info to the database dump, or to properly attribute memory
  // usage in memory dump providers that also dump |tracked_db|.
  // Note that |tracked_db| should be a live database instance produced by
  // OpenDatabase() method or leveldb_env::OpenDB() function.
  static base::trace_event::MemoryAllocatorDump* GetOrCreateAllocatorDump(
      base::trace_event::ProcessMemoryDump* pmd,
      leveldb::DB* tracked_db);

  // Provides extra information about a tracked database.
  class TrackedDB : public leveldb::DB {
   public:
    // Name that OpenDatabase() was called with.
    virtual const std::string& name() const = 0;
  };

  // Opens a database and starts tracking it. As long as the opened database
  // is alive (i.e. its instance is not destroyed) the database is exposed to
  // memory-infra and is enumerated by VisitDatabases() method.
  // This function is an implementation detail of leveldb_env::OpenDB(), and
  // has similar guarantees regarding |dbptr| argument.
  leveldb::Status OpenDatabase(const leveldb::Options& options,
                               const std::string& name,
                               TrackedDB** dbptr);

 private:
  class MemoryDumpProvider;
  class TrackedDBImpl;

  using DatabaseVisitor = base::RepeatingCallback<void(TrackedDB*)>;

  friend class ChromiumEnvDBTrackerTest;
  FRIEND_TEST_ALL_PREFIXES(ChromiumEnvDBTrackerTest, IsTrackedDB);

  DBTracker();
  ~DBTracker();

  static base::trace_event::MemoryAllocatorDump* GetOrCreateAllocatorDump(
      base::trace_event::ProcessMemoryDump* pmd,
      TrackedDB* db);

  // Calls |visitor| for each live database. The database is live from the
  // point it was returned from OpenDatabase() and up until its instance is
  // destroyed.
  // The databases may be visited in an arbitrary order.
  // This function takes a lock, preventing any database from being opened or
  // destroyed (but doesn't lock the databases themselves).
  void VisitDatabases(const DatabaseVisitor& visitor);

  // Checks if |db| is tracked.
  bool IsTrackedDB(const leveldb::DB* db) const;

  void DatabaseOpened(TrackedDBImpl* database);
  void DatabaseDestroyed(TrackedDBImpl* database);

  std::unique_ptr<MemoryDumpProvider> mdp_;

  mutable base::Lock databases_lock_;
  base::LinkedList<TrackedDBImpl> databases_;

  DISALLOW_COPY_AND_ASSIGN(DBTracker);
};

// Determine the appropriate leveldb write buffer size to use. The default size
// (4MB) may result in a log file too large to be compacted given the available
// storage space. This function will return smaller values for smaller disks,
// and the default leveldb value for larger disks.
//
// |disk_space| is the logical partition size (in bytes), and *not* available
// space. A value of -1 will return leveldb's default write buffer size.
LEVELDB_EXPORT size_t WriteBufferSize(int64_t disk_space);

// Opens a database and exposes it to Chrome's tracing (see DBTracker for
// details). The function guarantees that:
//   1. |dbptr| is not touched on failure
//   2. |dbptr| is not NULL on success
LEVELDB_EXPORT leveldb::Status OpenDB(const leveldb_chrome::Options& options,
                                      const std::string& name,
                                      std::unique_ptr<leveldb::DB>* dbptr);

// Return the shared leveldb block cache for web APIs. The caller *does not*
// own the returned instance.
LEVELDB_EXPORT leveldb::Cache* GetSharedWebBlockCache();

// Return the shared leveldb block cache for browser (non web) APIs. The caller
// *does not* own the returned instance.
LEVELDB_EXPORT leveldb::Cache* GetSharedBrowserBlockCache();

}  // namespace leveldb_chrome

#endif  // THIRD_PARTY_LEVELDATABASE_LEVELDB_CHROME_H_
