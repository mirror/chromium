// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "third_party/leveldatabase/leveldb_chrome.h"

#include <memory>

#include "base/format_macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/sys_info.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/memory_dump_provider.h"
#include "base/trace_event/process_memory_dump.h"
#include "base/trace_event/trace_event.h"
#include "leveldb/cache.h"

using leveldb::Cache;
using leveldb::NewLRUCache;

namespace leveldb_chrome {

namespace {

size_t DefaultBlockCacheSize() {
  if (base::SysInfo::IsLowEndDevice())
    return 1 << 20;  // 1MB
  else
    return 8 << 20;  // 8MB
}

// Singleton owning resources shared by Chrome's leveldb databases.
class Globals {
 public:
  static Globals* GetInstance() {
    static Globals* globals = new Globals();
    return globals;
  }

  Globals() : browser_block_cache_(NewLRUCache(DefaultBlockCacheSize())) {
    if (!base::SysInfo::IsLowEndDevice())
      web_block_cache_.reset(NewLRUCache(DefaultBlockCacheSize()));
  }

  Cache* web_block_cache() const {
    if (web_block_cache_)
      return web_block_cache_.get();
    return browser_block_cache();
  }

  Cache* browser_block_cache() const { return browser_block_cache_.get(); }

 private:
  ~Globals() {}

  std::unique_ptr<Cache> web_block_cache_;      // null on low end devices.
  std::unique_ptr<Cache> browser_block_cache_;  // Never null.

  DISALLOW_COPY_AND_ASSIGN(Globals);
};

}  // namespace

Options::Options() {
// Note: Ensure that these default values correspond to those in
// components/leveldb/public/interfaces/leveldb.mojom.
// TODO(cmumford) Create struct-trait for leveldb.mojom.OpenOptions to force
// users to pass in a leveldb_chrome::Options instance (and it's defaults).
//
// Currently log reuse is an experimental feature in leveldb. More info at:
// https://github.com/google/leveldb/commit/251ebf5dc70129ad3
#if defined(OS_CHROMEOS)
  // Reusing logs on Chrome OS resulted in an unacceptably high leveldb
  // corruption rate (at least for Indexed DB). More info at
  // https://crbug.com/460568
  reuse_logs = false;
#else
  reuse_logs = true;
#endif
  // By default use a single shared block cache to conserve memory. The owner of
  // this object can create their own, or set to NULL to have leveldb create a
  // new db-specific block cache.
  block_cache = leveldb_chrome::GetSharedBrowserBlockCache();
}

// Forwards all calls to the underlying leveldb::DB instance.
// Adds / removes itself in the DBTracker it's created with.
class DBTracker::TrackedDBImpl : public base::LinkNode<TrackedDBImpl>,
                                 public TrackedDB {
 public:
  TrackedDBImpl(DBTracker* tracker, const std::string name, leveldb::DB* db)
      : tracker_(tracker), name_(name), db_(db) {
    tracker_->DatabaseOpened(this);
  }

  ~TrackedDBImpl() override { tracker_->DatabaseDestroyed(this); }

  const std::string& name() const override { return name_; }

  leveldb::Status Put(const leveldb::WriteOptions& options,
                      const leveldb::Slice& key,
                      const leveldb::Slice& value) override {
    return db_->Put(options, key, value);
  }

  leveldb::Status Delete(const leveldb::WriteOptions& options,
                         const leveldb::Slice& key) override {
    return db_->Delete(options, key);
  }

  leveldb::Status Write(const leveldb::WriteOptions& options,
                        leveldb::WriteBatch* updates) override {
    return db_->Write(options, updates);
  }

  leveldb::Status Get(const leveldb::ReadOptions& options,
                      const leveldb::Slice& key,
                      std::string* value) override {
    return db_->Get(options, key, value);
  }

  const leveldb::Snapshot* GetSnapshot() override { return db_->GetSnapshot(); }

  void ReleaseSnapshot(const leveldb::Snapshot* snapshot) override {
    return db_->ReleaseSnapshot(snapshot);
  }

  bool GetProperty(const leveldb::Slice& property,
                   std::string* value) override {
    return db_->GetProperty(property, value);
  }

  void GetApproximateSizes(const leveldb::Range* range,
                           int n,
                           uint64_t* sizes) override {
    return db_->GetApproximateSizes(range, n, sizes);
  }

  void CompactRange(const leveldb::Slice* begin,
                    const leveldb::Slice* end) override {
    return db_->CompactRange(begin, end);
  }

  leveldb::Iterator* NewIterator(const leveldb::ReadOptions& options) override {
    return db_->NewIterator(options);
  }

 private:
  DBTracker* tracker_;
  std::string name_;
  std::unique_ptr<leveldb::DB> db_;
};

// Reports live databases to memory-infra. For each live database the following
// information is reported:
// 1. Instance pointer (to disambiguate databases).
// 2. Memory taken by the database.
// 3. The name of the database (when not in BACKGROUND mode to avoid exposing
//    PIIs in slow reports).
//
// Example report (as seen after clicking "leveldatabase" in "Overview" pane
// in Chrome tracing UI):
//
// Component             size          name
// ---------------------------------------------------------------------------
// leveldatabase         204.4 KiB
//   0x7FE70F2040A0      4.0 KiB       /Users/.../data_reduction_proxy_leveldb
//   0x7FE70F530D80      188.4 KiB     /Users/.../Sync Data/LevelDB
//   0x7FE71442F270      4.0 KiB       /Users/.../Sync App Settings/...
//   0x7FE71471EC50      8.0 KiB       /Users/.../Extension State
//
class DBTracker::MemoryDumpProvider
    : public base::trace_event::MemoryDumpProvider {
 public:
  bool OnMemoryDump(const base::trace_event::MemoryDumpArgs& args,
                    base::trace_event::ProcessMemoryDump* pmd) override {
    // Don't dump in background mode ("from the field") until whitelisted.
    if (args.level_of_detail ==
        base::trace_event::MemoryDumpLevelOfDetail::BACKGROUND) {
      return true;
    }

    auto db_visitor = [](const base::trace_event::MemoryDumpArgs& args,
                         base::trace_event::ProcessMemoryDump* pmd,
                         TrackedDB* db) {
      auto* dump = DBTracker::GetOrCreateAllocatorDump(pmd, db);
      // TODO(ssid): Do not add string attribute in background mode.
      dump->AddString("name", "", db->name());
    };

    DBTracker::GetInstance()->VisitDatabases(
        base::BindRepeating(db_visitor, args, base::Unretained(pmd)));
    return true;
  }
};

DBTracker::DBTracker() : mdp_(new MemoryDumpProvider()) {
  base::trace_event::MemoryDumpManager::GetInstance()->RegisterDumpProvider(
      mdp_.get(), "LevelDB", nullptr);
}

DBTracker::~DBTracker() {
  NOTREACHED();  // DBTracker is a singleton
}

// static
DBTracker* DBTracker::GetInstance() {
  static DBTracker* instance = new DBTracker();
  return instance;
}

// static
base::trace_event::MemoryAllocatorDump* DBTracker::GetOrCreateAllocatorDump(
    base::trace_event::ProcessMemoryDump* pmd,
    leveldb::DB* tracked_db) {
  DCHECK(GetInstance()->IsTrackedDB(tracked_db))
      << std::hex << tracked_db << " is not tracked";
  return GetOrCreateAllocatorDump(pmd,
                                  reinterpret_cast<TrackedDB*>(tracked_db));
}

// static
base::trace_event::MemoryAllocatorDump* DBTracker::GetOrCreateAllocatorDump(
    base::trace_event::ProcessMemoryDump* pmd,
    TrackedDB* db) {
  if (pmd->dump_args().level_of_detail ==
      base::trace_event::MemoryDumpLevelOfDetail::BACKGROUND) {
    return nullptr;
  }
  std::string dump_name = base::StringPrintf("leveldatabase/0x%" PRIXPTR,
                                             reinterpret_cast<uintptr_t>(db));
  auto* dump = pmd->GetAllocatorDump(dump_name);
  if (dump)
    return dump;
  dump = pmd->CreateAllocatorDump(dump_name);

  uint64_t memory_usage = 0;
  std::string usage_string;
  bool success =
      db->GetProperty("leveldb.approximate-memory-usage", &usage_string) &&
      base::StringToUint64(usage_string, &memory_usage);
  DCHECK(success);
  dump->AddScalar(base::trace_event::MemoryAllocatorDump::kNameSize,
                  base::trace_event::MemoryAllocatorDump::kUnitsBytes,
                  memory_usage);

  const char* system_allocator_name =
      base::trace_event::MemoryDumpManager::GetInstance()
          ->system_allocator_pool_name();
  if (system_allocator_name)
    pmd->AddSuballocation(dump->guid(), system_allocator_name);
  return dump;
}

bool DBTracker::IsTrackedDB(const leveldb::DB* db) const {
  base::AutoLock lock(databases_lock_);
  for (auto* i = databases_.head(); i != databases_.end(); i = i->next()) {
    if (i->value() == db)
      return true;
  }
  return false;
}

leveldb::Status DBTracker::OpenDatabase(const leveldb::Options& options,
                                        const std::string& name,
                                        TrackedDB** dbptr) {
  leveldb::DB* db = nullptr;
  auto status = leveldb::DB::Open(options, name, &db);
  // Enforce expectations: either we succeed, and get a valid object in |db|,
  // or we fail, and |db| is still NULL.
  CHECK((status.ok() && db) || (!status.ok() && !db));
  if (status.ok()) {
    // TrackedDBImpl ctor adds the instance to the tracker.
    *dbptr = new TrackedDBImpl(GetInstance(), name, db);
  }
  return status;
}

void DBTracker::VisitDatabases(const DatabaseVisitor& visitor) {
  base::AutoLock lock(databases_lock_);
  for (auto* i = databases_.head(); i != databases_.end(); i = i->next()) {
    visitor.Run(i->value());
  }
}

void DBTracker::DatabaseOpened(TrackedDBImpl* database) {
  base::AutoLock lock(databases_lock_);
  databases_.Append(database);
}

void DBTracker::DatabaseDestroyed(TrackedDBImpl* database) {
  base::AutoLock lock(databases_lock_);
  database->RemoveFromList();
}

leveldb::Status OpenDB(const leveldb_chrome::Options& options,
                       const std::string& name,
                       std::unique_ptr<leveldb::DB>* dbptr) {
  DBTracker::TrackedDB* tracked_db = nullptr;
  leveldb::Status status =
      DBTracker::GetInstance()->OpenDatabase(options, name, &tracked_db);
  if (status.ok()) {
    dbptr->reset(tracked_db);
  }
  return status;
}

// Returns a separate (from the default) block cache for use by web APIs.
// This must be used when opening the databases accessible to Web-exposed APIs,
// so rogue pages can't mount a denial of service attack by hammering the block
// cache. Without separate caches, such an attack might slow down Chrome's UI to
// the point where the user can't close the offending page's tabs.
Cache* GetSharedWebBlockCache() {
  return Globals::GetInstance()->web_block_cache();
}

Cache* GetSharedBrowserBlockCache() {
  return Globals::GetInstance()->browser_block_cache();
}

// Given the size of the disk, identified by |disk_size| in bytes, determine the
// appropriate write_buffer_size. Ignoring snapshots, if the current set of
// tables in a database contains a set of key/value pairs identified by {A}, and
// a set of key/value pairs identified by {B} has been written and is in the log
// file, then during compaction you will have {A} + {B} + {A, B} = 2A + 2B.
// There is no way to know the size of A, so minimizing the size of B will
// maximize the likelihood of a successful compaction.
size_t WriteBufferSize(int64_t disk_size) {
  const leveldb_chrome::Options default_options;
  const int64_t kMinBufferSize = 1024 * 1024;
  const int64_t kMaxBufferSize = default_options.write_buffer_size;
  const int64_t kDiskMinBuffSize = 10 * 1024 * 1024;
  const int64_t kDiskMaxBuffSize = 40 * 1024 * 1024;

  if (disk_size == -1)
    return default_options.write_buffer_size;

  if (disk_size <= kDiskMinBuffSize)
    return kMinBufferSize;

  if (disk_size >= kDiskMaxBuffSize)
    return kMaxBufferSize;

  // A linear equation to intersect (kDiskMinBuffSize, kMinBufferSize) and
  // (kDiskMaxBuffSize, kMaxBufferSize).
  return static_cast<size_t>(
      kMinBufferSize +
      ((kMaxBufferSize - kMinBufferSize) * (disk_size - kDiskMinBuffSize)) /
          (kDiskMaxBuffSize - kDiskMinBuffSize));
}

}  // namespace leveldb_chrome
