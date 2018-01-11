// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/dom_storage/session_storage_context_mojo.h"

#include <inttypes.h>
#include <cctype>  // for std::isalnum
#include <cstring>
#include <utility>

#include "base/barrier_closure.h"
#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/sys_info.h"
#include "base/trace_event/memory_dump_manager.h"
#include "build/build_config.h"
#include "components/leveldb/public/cpp/util.h"
#include "components/leveldb/public/interfaces/leveldb.mojom.h"
#include "content/browser/leveldb_wrapper_impl.h"
#include "content/common/dom_storage/dom_storage_types.h"
#include "content/public/browser/session_storage_usage_info.h"
#include "content/public/common/content_features.h"
#include "services/file/public/interfaces/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "third_party/leveldatabase/env_chromium.h"
#include "third_party/leveldatabase/leveldb_chrome.h"
#include "url/gurl.h"

namespace content {
namespace {
constexpr const char kNamespacePrefix[] = "namespace-";
constexpr const size_t kNamespacePrefixLength = 10;
constexpr const size_t kPersistentSessionIdLength = 36;
//constexpr const char kNamespaceOriginSeperator = '-';
constexpr const size_t kNamespaceOriginSeperatorLength = 1;
constexpr const size_t kPrefixBeforeOriginLength =
    kNamespacePrefixLength + kPersistentSessionIdLength +
    kNamespaceOriginSeperatorLength;

const char kSessionStorageVersionKey[] = "VERSION";
const int64_t kMinSessionStorageSchemaVersion = 1;
const int64_t kCurrentSessionStorageSchemaVersion = 1;
// After this many consecutive commit errors we'll throw away the entire
// database.
const int kCommitErrorThreshold = 8;

enum class CachePurgeReason {
  NotNeeded,
  SizeLimitExceeded,
  AreaCountLimitExceeded,
  InactiveOnLowEndDevice,
  AggressivePurgeTriggered
};

void RecordCachePurgedHistogram(CachePurgeReason reason,
                                size_t purged_size_kib) {
  UMA_HISTOGRAM_COUNTS_100000("LocalStorageContext.CachePurgedInKB",
                              purged_size_kib);
  switch (reason) {
    case CachePurgeReason::SizeLimitExceeded:
      UMA_HISTOGRAM_COUNTS_100000(
          "LocalStorageContext.CachePurgedInKB.SizeLimitExceeded",
          purged_size_kib);
      break;
    case CachePurgeReason::AreaCountLimitExceeded:
      UMA_HISTOGRAM_COUNTS_100000(
          "LocalStorageContext.CachePurgedInKB.AreaCountLimitExceeded",
          purged_size_kib);
      break;
    case CachePurgeReason::InactiveOnLowEndDevice:
      UMA_HISTOGRAM_COUNTS_100000(
          "LocalStorageContext.CachePurgedInKB.InactiveOnLowEndDevice",
          purged_size_kib);
      break;
    case CachePurgeReason::AggressivePurgeTriggered:
      UMA_HISTOGRAM_COUNTS_100000(
          "LocalStorageContext.CachePurgedInKB.AggressivePurgeTriggered",
          purged_size_kib);
      break;
    case CachePurgeReason::NotNeeded:
      NOTREACHED();
      break;
  }
}

void NoOpSuccess(bool success) {}

bool ValueToMapNumber(const std::vector<uint8_t>& value, int64_t* out) {
  const base::StringPiece piece(reinterpret_cast<const char*>(&(value[0])),
                                value.size());
  return base::StringToInt64(piece, out);
}

std::vector<uint8_t> MapNumberToValue(int64_t map_number) {
  return leveldb::StdStringToUint8Vector(base::NumberToString(map_number));
}

}  // namespace

class SessionStorageContextMojo::LevelDBRefCountedMap final
    : public LevelDBWrapperImpl::Delegate {
 public:
  LevelDBRefCountedMap(SessionStorageContextMojo* context,
                       std::vector<uint8_t> map_prefix)
      : context_(context),
        wrapper_impl_(std::make_unique<LevelDBWrapperImpl>(
            context_->database_.get(),
            std::move(map_prefix),
            this,
            GetOptions())),
        level_db_wrapper_ptr_(wrapper_impl_.get()) {
  }
  // Constructor for forking.
  LevelDBRefCountedMap(std::vector<uint8_t> map_prefix,
                       LevelDBRefCountedMap* forking_from)
      : context_(forking_from->context_),
        wrapper_impl_(forking_from->level_db_wrapper()->ForkToNewPrefix(
            std::move(map_prefix),
            this,
            GetOptions())),
        level_db_wrapper_ptr_(wrapper_impl_.get()) {
   }

  ~LevelDBRefCountedMap() override = default;

  int map_refcount() { return map_refcount_; }

  void AddMapReference() { ++map_refcount_; }

  void RemoveMapReference() {
    static constexpr const char kLevelDBRefCountedMapSource[] =
        "LevelDBRefCountedMap";
    DCHECK_GT(map_refcount_, 0);
    --map_refcount_;
    if (map_refcount_ <= 0) {
      wrapper_impl_->DeleteAll(kLevelDBRefCountedMapSource,
                               base::BindOnce(&NoOpSuccess));
    }
  }

  LevelDBWrapperImpl* level_db_wrapper() {
    return level_db_wrapper_ptr_;
  }

  // Note: this is irrelevant, as this context has its own binding handling.
  /// xxx: make a way to wait until load is complete, so we don't destroy our
  // wrappers before changes are saved.
  void OnNoBindings() override {}

  std::vector<leveldb::mojom::BatchedOperationPtr> PrepareToCommit() override {
    std::vector<leveldb::mojom::BatchedOperationPtr> operations;

    // Write schema version if not already done so before.
    if (!context_->database_initialized_) {
      leveldb::mojom::BatchedOperationPtr item =
          leveldb::mojom::BatchedOperation::New();
      item->type = leveldb::mojom::BatchOperationType::PUT_KEY;
      item->key = leveldb::StdStringToUint8Vector(kSessionStorageVersionKey);
      item->value = leveldb::StdStringToUint8Vector(
          base::Int64ToString(kCurrentSessionStorageSchemaVersion));
      operations.push_back(std::move(item));
      context_->database_initialized_ = true;
    }
//
//    leveldb::mojom::BatchedOperationPtr item =
//        leveldb::mojom::BatchedOperation::New();
//    item->type = leveldb::mojom::BatchOperationType::PUT_KEY;
//    item->key = CreateMetaDataKey(origin_);
//    if (ref_counted_map()->impl()->empty()) {
//      item->type = leveldb::mojom::BatchOperationType::DELETE_KEY;
//    } else {
//      item->type = leveldb::mojom::BatchOperationType::PUT_KEY;
//      LocalStorageOriginMetaData data;
//      data.set_last_modified(base::Time::Now().ToInternalValue());
//      data.set_size_bytes(ref_counted_map()->impl()->storage_used());
//      item->value = leveldb::StdStringToUint8Vector(data.SerializeAsString());
//    }
//    operations.push_back(std::move(item));
    // xxx figure out how to store size.

    return operations;
  }

  void DidCommit(leveldb::mojom::DatabaseError error) override {
    UMA_HISTOGRAM_ENUMERATION("SessionStorageContext.CommitResult",
                              leveldb::GetLevelDBStatusUMAValue(error),
                              leveldb_env::LEVELDB_STATUS_MAX);
    context_->OnCommitResult(error);
  }

 private:

   LevelDBWrapperImpl::Options GetOptions() {
     // Delay for a moment after a value is set in anticipation
     // of other values being set, so changes are batched.
     constexpr const base::TimeDelta kCommitDefaultDelaySecs =
         base::TimeDelta::FromSeconds(5);

     // To avoid excessive IO we apply limits to the amount of data being
     // written and the frequency of writes.
     const int kMaxBytesPerHour = kPerStorageAreaQuota;
     const int kMaxCommitsPerHour = 60;

     LevelDBWrapperImpl::Options options;
     options.max_size =
         kPerStorageAreaQuota + kPerStorageAreaOverQuotaAllowance;
     options.default_commit_delay = kCommitDefaultDelaySecs;
     options.max_bytes_per_hour = kMaxBytesPerHour;
     options.max_commits_per_hour = kMaxCommitsPerHour;
#if defined(OS_ANDROID)
     options.cache_mode =
         LevelDBWrapperImpl::CacheMode::KEYS_ONLY_WHEN_POSSIBLE;
#else
     options.cache_mode = LevelDBWrapperImpl::CacheMode::KEYS_AND_VALUES;
     if (base::SysInfo::IsLowEndDevice()) {
       options.cache_mode =
           LevelDBWrapperImpl::CacheMode::KEYS_ONLY_WHEN_POSSIBLE;
     }
#endif
     return options;
   }

   SessionStorageContextMojo* context_;

   int map_refcount_ = 0;
   std::unique_ptr<LevelDBWrapperImpl> wrapper_impl_;
   // Holds the same value as |level_db_wrapper_|. The reason for this is that
   // during destruction of the LevelDBWrapperImpl instance we might still get
   // called and need access  to the LevelDBWrapperImpl instance. The unique_ptr
   // could already be null, but this field should still be valid.
   // TODO(dmurph): Change delegate ownership so this doesn't have to be done.
   LevelDBWrapperImpl* level_db_wrapper_ptr_;

   DISALLOW_COPY_AND_ASSIGN(LevelDBRefCountedMap);
};

class SessionStorageContextMojo::LevelDBWrapperBinder
    : public mojom::LevelDBWrapper {
 public:
  LevelDBWrapperBinder(SessionStorageContextMojo* context,
                       std::string persistent_namespace_id,
                       LevelDBRefCountedMap* map)
      : context_(context),
        persistent_namespace_id_(persistent_namespace_id),
        refcounted_map_(map),
        binding_(this) {
    binding_.set_connection_error_handler(base::BindOnce(
        &LevelDBWrapperBinder::OnConnectionError, base::Unretained(this)));
    refcounted_map_->AddMapReference();
  }

  ~LevelDBWrapperBinder() override {
    refcounted_map_->RemoveMapReference();
  }

  void Bind(mojom::LevelDBWrapperRequest request) {
    binding_.Bind(std::move(request));
  }

  LevelDBRefCountedMap* refcounted_map() { return refcounted_map_; }

  void OnConnectionError() {
    LOG(ERROR) << "connection error!";
  }

  // LevelDBWrapper:
  void AddObserver(mojom::LevelDBObserverAssociatedPtrInfo observer) override {
    mojom::LevelDBObserverAssociatedPtr observer_ptr;
    observer_ptr.Bind(std::move(observer));
    if (refcounted_map_->level_db_wrapper()->cache_mode() ==
        LevelDBWrapperImpl::CacheMode::KEYS_AND_VALUES)
      observer_ptr->ShouldSendOldValueOnMutations(false);
    mojo::PtrId ptr_id = refcounted_map_->level_db_wrapper()->AddObserver(
        std::move(observer_ptr));
    observer_ptrs_.push_back(ptr_id);
  }
  void Put(const std::vector<uint8_t>& key,
           const std::vector<uint8_t>& value,
           const base::Optional<std::vector<uint8_t>>& client_old_value,
           const std::string& source,
           PutCallback callback) override {
    if (refcounted_map_->map_refcount() > 1)
      ForkToNewMap();
    refcounted_map_->level_db_wrapper()->Put(key, value, client_old_value,
                                             source, std::move(callback));
  }
  void Delete(const std::vector<uint8_t>& key,
              const base::Optional<std::vector<uint8_t>>& client_old_value,
              const std::string& source,
              DeleteCallback callback) override {
    if (refcounted_map_->map_refcount() > 1)
      ForkToNewMap();
    refcounted_map_->level_db_wrapper()->Delete(key, client_old_value, source,
                                                std::move(callback));
  }
  void DeleteAll(const std::string& source,
                 DeleteAllCallback callback) override {
    if (refcounted_map_->map_refcount() > 1)
      ForkToNewMap();
    refcounted_map_->level_db_wrapper()->DeleteAll(source, std::move(callback));
  }
  void Get(const std::vector<uint8_t>& key, GetCallback callback) override {
    refcounted_map_->level_db_wrapper()->Get(key, std::move(callback));
  }
  void GetAll(
      mojom::LevelDBWrapperGetAllCallbackAssociatedPtrInfo complete_callback,
      GetAllCallback callback) override {
    refcounted_map_->level_db_wrapper()->GetAll(std::move(complete_callback),
                                                std::move(callback));
  }

 private:
  void ForkToNewMap() {
    std::vector<mojom::LevelDBObserverAssociatedPtr> ptrs_to_move;
    for (const mojo::PtrId& ptr_id : observer_ptrs_) {
      DCHECK(refcounted_map_->level_db_wrapper()->HasObserver(ptr_id));
      ptrs_to_move.push_back(
          refcounted_map_->level_db_wrapper()->RemoveObserver(ptr_id));
    }
    observer_ptrs_.clear();
    // TODO(dmurph): Move to vector prefix version when cleaned up.
    std::vector<uint8_t> new_prefix = context_->GetNextMapPrefix();
    std::unique_ptr<LevelDBRefCountedMap> fork =
        std::make_unique<LevelDBRefCountedMap>(new_prefix, refcounted_map_);
    refcounted_map_->RemoveMapReference();
    refcounted_map_ = fork.get();
    refcounted_map_->AddMapReference();
    DCHECK_EQ(1, refcounted_map_->map_refcount());

    context_->data_maps_.emplace(std::move(new_prefix), std::move(fork));
    for (auto& observer_ptr : ptrs_to_move) {
      observer_ptrs_.push_back(refcounted_map_->level_db_wrapper()->AddObserver(
          std::move(observer_ptr)));
    }
  }

  SessionStorageContextMojo* context_;
  std::string persistent_namespace_id_;
  LevelDBRefCountedMap* refcounted_map_;

  std::vector<mojo::PtrId> observer_ptrs_;
  mojo::Binding<mojom::LevelDBWrapper> binding_;

  DISALLOW_COPY_AND_ASSIGN(LevelDBWrapperBinder);
};


SessionStorageContextMojo::SessionStorageContextMojo(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    service_manager::Connector* connector,
    base::FilePath partition_directory,
    std::string leveldb_name)
    : connector_(connector ? connector->Clone() : nullptr),
      partition_directory_path_(std::move(partition_directory)),
      leveldb_name_(std::move(leveldb_name)),
      memory_dump_id_(base::StringPrintf("SessionStorage/0x%" PRIXPTR,
                                         reinterpret_cast<uintptr_t>(this))),
      is_low_end_device_(base::SysInfo::IsLowEndDevice()),
      weak_ptr_factory_(this) {
  DCHECK(base::FeatureList::IsEnabled(features::kMojoSessionStorage));
  base::trace_event::MemoryDumpManager::GetInstance()->RegisterDumpProvider(
      this, "SessionStorage", task_runner);
}

SessionStorageContextMojo::~SessionStorageContextMojo() {
  DCHECK_EQ(connection_state_, CONNECTION_SHUTDOWN);
  base::trace_event::MemoryDumpManager::GetInstance()->UnregisterDumpProvider(
      this);
}

void SessionStorageContextMojo::OpenSessionStorage(
    int64_t namespace_id,
    const url::Origin& origin,
    mojom::LevelDBWrapperRequest request) {
  LOG(ERROR) << "openning namespace " << namespace_id;
  DCHECK(namespaces_.find(namespace_id) != namespaces_.end());
  std::string namespace_str_id = namespaces_[namespace_id];
  RunWhenConnected(
      base::BindOnce(&SessionStorageContextMojo::BindSessionStorage,
                     base::Unretained(this), std::move(namespace_str_id), origin,
                     std::move(request)));
}

void SessionStorageContextMojo::CreateSessionNamespace(
    int64_t namespace_id,
    std::string persistent_namespace_id) {
  LOG(ERROR) << "creating namespace " << namespace_id << " with p "
             << persistent_namespace_id;
  DCHECK(namespaces_.find(namespace_id) == namespaces_.end());
  namespaces_[namespace_id] = std::move(persistent_namespace_id);
}

void SessionStorageContextMojo::CloneSessionNamespace(
    int64_t namespace_id_to_clone,
    int64_t clone_namespace_id,
    std::string clone_persistent_namespace_id) {
  LOG(ERROR) << "cloning namespace " << namespace_id_to_clone << " to "
             << clone_namespace_id;
  DCHECK(namespaces_.find(namespace_id_to_clone) != namespaces_.end());
  DCHECK(namespaces_.find(clone_namespace_id) == namespaces_.end());

  const std::string& namespace_pid = namespaces_[namespace_id_to_clone];

  auto map_it = area_map_.upper_bound(
      std::pair<std::string, url::Origin>(namespace_pid, url::Origin()));

  std::vector<SessionStorageMap::value_type> to_insert;
  for (; map_it != area_map_.end() && map_it->first.first == namespace_pid;
       map_it++) {
    to_insert.push_back(std::make_pair(
        std::make_pair(clone_persistent_namespace_id, map_it->first.second),
        std::make_unique<LevelDBWrapperBinder>(
            this, clone_persistent_namespace_id,
            map_it->second->refcounted_map())));
  }
  using insert_iterator = std::vector<SessionStorageMap::value_type>::iterator;
  area_map_.insert(std::move_iterator<insert_iterator>(to_insert.begin()),
                   std::move_iterator<insert_iterator>(to_insert.end()));

  namespaces_[clone_namespace_id] = std::move(clone_persistent_namespace_id);
}

void SessionStorageContextMojo::DeleteSessionNamespace(int64_t namespace_id,
                                                       bool should_persist) {}

void SessionStorageContextMojo::Flush() {
  if (connection_state_ != CONNECTION_FINISHED) {
    RunWhenConnected(base::BindOnce(&SessionStorageContextMojo::Flush,
                                    weak_ptr_factory_.GetWeakPtr()));
    return;
  }
  // xxx flush wrappers
}

void SessionStorageContextMojo::GetStorageUsage(
    GetStorageUsageCallback callback) {
  RunWhenConnected(
      base::BindOnce(&SessionStorageContextMojo::RetrieveStorageUsage,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void SessionStorageContextMojo::DeleteStorage(
    const GURL& origin,
    const std::string& persistent_namespace_id) {
  if (connection_state_ != CONNECTION_FINISHED) {
    RunWhenConnected(base::BindOnce(&SessionStorageContextMojo::DeleteStorage,
                                    weak_ptr_factory_.GetWeakPtr(), origin,
                                    persistent_namespace_id));
    return;
  }

  // xxx find wrapper for namespace & origin and delete that.
}

void SessionStorageContextMojo::ShutdownAndDelete() {
  DCHECK_NE(connection_state_, CONNECTION_SHUTDOWN);

  // Nothing to do if no connection to the database was ever finished.
  if (connection_state_ != CONNECTION_FINISHED) {
    connection_state_ = CONNECTION_SHUTDOWN;
    OnShutdownComplete(leveldb::mojom::DatabaseError::OK);
    return;
  }

  connection_state_ = CONNECTION_SHUTDOWN;

  // Delete everything.
  // xxx

  OnShutdownComplete(leveldb::mojom::DatabaseError::OK);
}

void SessionStorageContextMojo::PurgeMemory() {
  // xxx
  size_t total_cache_size, unused_wrapper_count;
  GetStatistics(&total_cache_size, &unused_wrapper_count);

  // xxx: Purge all wrappers that don't have bindings.

  // Track the size of cache purged.
  size_t final_total_cache_size;
  GetStatistics(&final_total_cache_size, &unused_wrapper_count);
  size_t purged_size_kib = (total_cache_size - final_total_cache_size) / 1024;
  RecordCachePurgedHistogram(CachePurgeReason::AggressivePurgeTriggered,
                             purged_size_kib);
}

void SessionStorageContextMojo::PurgeUnusedWrappersIfNeeded() {
  // xxx
}

bool SessionStorageContextMojo::OnMemoryDump(
    const base::trace_event::MemoryDumpArgs& args,
    base::trace_event::ProcessMemoryDump* pmd) {
  // xxx
  return true;
}

void SessionStorageContextMojo::RunWhenConnected(base::OnceClosure callback) {
  DCHECK_NE(connection_state_, CONNECTION_SHUTDOWN);

  // If we don't have a filesystem_connection_, we'll need to establish one.
  if (connection_state_ == NO_CONNECTION) {
    connection_state_ = CONNECTION_IN_PROGRESS;
    InitiateConnection();
  }

  if (connection_state_ == CONNECTION_IN_PROGRESS) {
    // Queue this OpenSessionStorage call for when we have a level db pointer.
    on_database_opened_callbacks_.push_back(std::move(callback));
    return;
  }

  std::move(callback).Run();
}

void SessionStorageContextMojo::InitiateConnection(bool in_memory_only) {
  DCHECK_EQ(connection_state_, CONNECTION_IN_PROGRESS);
  // Unit tests might not always have a Connector, use in-memory only if that
  // happens.
  if (!connector_) {
    OnDatabaseOpened(false, leveldb::mojom::DatabaseError::OK);
    return;
  }

  if (!partition_directory_path_.empty() && !in_memory_only) {
    // We were given a subdirectory to write to. Get it and use a disk backed
    // database.
    connector_->BindInterface(file::mojom::kServiceName, &file_system_);
    file_system_->GetSubDirectory(
        partition_directory_path_.AsUTF8Unsafe(),
        MakeRequest(&partition_directory_),
        base::BindOnce(&SessionStorageContextMojo::OnDirectoryOpened,
                       weak_ptr_factory_.GetWeakPtr()));
  } else {
    // We were not given a subdirectory. Use a memory backed database.
    connector_->BindInterface(file::mojom::kServiceName, &leveldb_service_);
    leveldb_service_->OpenInMemory(
        memory_dump_id_, MakeRequest(&database_),
        base::BindOnce(&SessionStorageContextMojo::OnDatabaseOpened,
                       weak_ptr_factory_.GetWeakPtr(), true));
  }
}

void SessionStorageContextMojo::OnDirectoryOpened(
    filesystem::mojom::FileError err) {
  if (err != filesystem::mojom::FileError::OK) {
    // We failed to open the directory; continue with startup so that we create
    // the |level_db_wrappers_|.
    UMA_HISTOGRAM_ENUMERATION("SessionStorageContext.DirectoryOpenError",
                              -static_cast<base::File::Error>(err),
                              -base::File::FILE_ERROR_MAX);
    LogDatabaseOpenResult(OpenResult::DIRECTORY_OPEN_FAILED);
    OnDatabaseOpened(false, leveldb::mojom::DatabaseError::OK);
    return;
  }

  // Now that we have a directory, connect to the LevelDB service and get our
  // database.
  connector_->BindInterface(file::mojom::kServiceName, &leveldb_service_);

  // We might still need to use the directory, so create a clone.
  filesystem::mojom::DirectoryPtr partition_directory_clone;
  partition_directory_->Clone(MakeRequest(&partition_directory_clone));

  leveldb_env::Options options;
  options.create_if_missing = true;
  options.max_open_files = 0;  // use minimum
  // Default write_buffer_size is 4 MB but that might leave a 3.999
  // memory allocation in RAM from a log file recovery.
  options.write_buffer_size = 64 * 1024;
  options.block_cache = leveldb_chrome::GetSharedWebBlockCache();
  leveldb_service_->OpenWithOptions(
      std::move(options), std::move(partition_directory_clone), leveldb_name_,
      memory_dump_id_, MakeRequest(&database_),
      base::BindOnce(&SessionStorageContextMojo::OnDatabaseOpened,
                     weak_ptr_factory_.GetWeakPtr(), false));
}

void SessionStorageContextMojo::OnDatabaseOpened(
    bool in_memory,
    leveldb::mojom::DatabaseError status) {
  if (status != leveldb::mojom::DatabaseError::OK) {
    UMA_HISTOGRAM_ENUMERATION("SessionStorageContext.DatabaseOpenError",
                              leveldb::GetLevelDBStatusUMAValue(status),
                              leveldb_env::LEVELDB_STATUS_MAX);
    if (in_memory) {
      UMA_HISTOGRAM_ENUMERATION(
          "SessionStorageContext.DatabaseOpenError.Memory",
          leveldb::GetLevelDBStatusUMAValue(status),
          leveldb_env::LEVELDB_STATUS_MAX);
    } else {
      UMA_HISTOGRAM_ENUMERATION("SessionStorageContext.DatabaseOpenError.Disk",
                                leveldb::GetLevelDBStatusUMAValue(status),
                                leveldb_env::LEVELDB_STATUS_MAX);
    }
    LogDatabaseOpenResult(OpenResult::DATABASE_OPEN_FAILED);
    // If we failed to open the database, try to delete and recreate the
    // database, or ultimately fallback to an in-memory database.
    DeleteAndRecreateDatabase(
        "SessionStorageContext.OpenResultAfterOpenFailed");
    return;
  }

  // Verify DB schema version.
  if (database_) {
    database_->Get(
        leveldb::StdStringToUint8Vector(kSessionStorageVersionKey),
        base::BindOnce(&SessionStorageContextMojo::OnGotDatabaseVersion,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  OnConnectionFinished();
}

void SessionStorageContextMojo::OnGotDatabaseVersion(
    leveldb::mojom::DatabaseError status,
    const std::vector<uint8_t>& value) {
  if (status == leveldb::mojom::DatabaseError::NOT_FOUND) {
    // New database, nothing more to do. Current version will get written
    // when first data is committed.
  } else if (status == leveldb::mojom::DatabaseError::OK) {
    // Existing database, check if version number matches current schema
    // version.
    int64_t db_version;
    if (!base::StringToInt64(leveldb::Uint8VectorToStdString(value),
                             &db_version) ||
        db_version < kMinSessionStorageSchemaVersion ||
        db_version > kCurrentSessionStorageSchemaVersion) {
      LogDatabaseOpenResult(OpenResult::INVALID_VERSION);
      DeleteAndRecreateDatabase(
          "SessionStorageContext.OpenResultAfterInvalidVersion");
      return;
    }
    database_initialized_ = true;
  } else {
    // Other read error. Possibly database corruption.
    UMA_HISTOGRAM_ENUMERATION("SessionStorageContext.ReadVersionError",
                              leveldb::GetLevelDBStatusUMAValue(status),
                              leveldb_env::LEVELDB_STATUS_MAX);
    LogDatabaseOpenResult(OpenResult::VERSION_READ_ERROR);
    DeleteAndRecreateDatabase(
        "SessionStorageContext.OpenResultAfterReadVersionError");
    return;
  }
  connection_state_ = FETCHING_METADATA;

  base::RepeatingClosure barrier = base::BarrierClosure(
      2, base::Bind(&SessionStorageContextMojo::OnConnectionFinished,
                    weak_ptr_factory_.GetWeakPtr()));
  database_->GetPrefixed(
      leveldb::StdStringToUint8Vector(kNamespacePrefix),
      base::BindOnce(&SessionStorageContextMojo::OnGotNamespaces,
                     weak_ptr_factory_.GetWeakPtr(), barrier));
}

void SessionStorageContextMojo::OnGotNamespaces(base::OnceClosure done,
    leveldb::mojom::DatabaseError status,
    std::vector<leveldb::mojom::KeyValuePtr> values) {
  if (connection_state_ != FETCHING_METADATA)
    return;
  if (status == leveldb::mojom::DatabaseError::NOT_FOUND) {
    LOG(ERROR) << "no namespaces found!";
    std::move(done).Run();
    return;
  }
  if (status != leveldb::mojom::DatabaseError::OK) {
    // Other read error. Possibly database corruption.
    UMA_HISTOGRAM_ENUMERATION("SessionStorageContext.ReadNamespacesError",
                              leveldb::GetLevelDBStatusUMAValue(status),
                              leveldb_env::LEVELDB_STATUS_MAX);
    LogDatabaseOpenResult(OpenResult::NAMESPACES_READ_ERROR);
    DeleteAndRecreateDatabase(
        "SessionStorageContext.OpenResultAfterReadNamespacesError");
    return;
  }

  for (const leveldb::mojom::KeyValuePtr& key_value : values) {
    const std::vector<uint8_t>& map_id = key_value->value;
    size_t key_size = key_value->key.size();
    if (key_size <= kNamespacePrefixLength)
      continue;
    if (key_size < kNamespacePrefixLength + kPersistentSessionIdLength)
      continue;

    std::string namespace_id(
        reinterpret_cast<char*>(&(key_value->key[kNamespacePrefixLength])),
        kPersistentSessionIdLength);
    if (key_size > kPrefixBeforeOriginLength) {
      std::string origin_str = std::string(
          reinterpret_cast<char*>(&(key_value->key[kPrefixBeforeOriginLength])),
          key_size - kPrefixBeforeOriginLength);

      AreaKey area_key(namespace_id, url::Origin::Create(GURL(origin_str)));

      DCHECK(area_map_.find(area_key) == area_map_.end());

      auto map_it = data_maps_.find(map_id);
      if (map_it == data_maps_.end()) {
        map_it = data_maps_
                     .emplace(map_id, std::make_unique<LevelDBRefCountedMap>(
                                          this, map_id))
                     .first;
      }
      LevelDBRefCountedMap* map_holder = map_it->second.get();

      int64_t map_number;
      if (ValueToMapNumber(map_id, &map_number) &&
          map_number >= next_map_id_from_namespaces_) {
        next_map_id_from_namespaces_ = map_number + 1;
        if (next_map_id_ == 0)
          next_map_id_ = next_map_id_from_namespaces_;
      }

      area_map_.insert(std::make_pair(
          std::move(area_key), std::make_unique<LevelDBWrapperBinder>(
                                   this, std::move(namespace_id), map_holder)));
    }
  }

  std::move(done).Run();
}
void SessionStorageContextMojo::OnGotNextMapId(base::OnceClosure done,
    leveldb::mojom::DatabaseError status,
    const std::vector<uint8_t>& map_id) {
  if (connection_state_ != FETCHING_METADATA)
    return;
  if (status == leveldb::mojom::DatabaseError::NOT_FOUND) {
    std::move(done).Run();
    return;
  }
  if (status == leveldb::mojom::DatabaseError::OK) {
    if (!ValueToMapNumber(map_id, &next_map_id_))
      next_map_id_ = next_map_id_from_namespaces_;
    std::move(done).Run();
    return;
  }

  // Other read error. Possibly database corruption.
  UMA_HISTOGRAM_ENUMERATION("SessionStorageContext.ReadNextMapIdError",
                            leveldb::GetLevelDBStatusUMAValue(status),
                            leveldb_env::LEVELDB_STATUS_MAX);
  LogDatabaseOpenResult(OpenResult::NAMESPACES_READ_ERROR);
  DeleteAndRecreateDatabase(
      "SessionStorageContext.OpenResultAfterReadNextMapIdError");
}

void SessionStorageContextMojo::OnConnectionFinished() {
  DCHECK_EQ(connection_state_, FETCHING_METADATA);
  LOG(ERROR) << "connection finished";
  if (!database_) {
    partition_directory_.reset();
    file_system_.reset();
    leveldb_service_.reset();
  }

  // If connection was opened successfully, reset tried_to_recreate_during_open_
  // to enable recreating the database on future errors.
  if (database_)
    tried_to_recreate_during_open_ = false;

  open_result_histogram_ = nullptr;

  // |database_| should be known to either be valid or invalid by now. Run our
  // delayed bindings.
  connection_state_ = CONNECTION_FINISHED;
  std::vector<base::OnceClosure> callbacks;
  std::swap(callbacks, on_database_opened_callbacks_);
  for (size_t i = 0; i < callbacks.size(); ++i)
    std::move(callbacks[i]).Run();
}

void SessionStorageContextMojo::DeleteAndRecreateDatabase(
    const char* histogram_name) {
  // We're about to set database_ to null, so delete the LevelDBWrappers
  // that might still be using the old database.
  // XXX(dmurph) clear wrappers
  //  for (const auto& it : level_db_wrappers_)
  //    it.second->level_db_wrapper()->CancelAllPendingRequests();
  //  level_db_wrappers_.clear();

  // Reset state to be in process of connecting. This will cause requests for
  // LevelDBWrappers to be queued until the connection is complete.
  connection_state_ = CONNECTION_IN_PROGRESS;
  commit_error_count_ = 0;
  database_ = nullptr;
  open_result_histogram_ = histogram_name;

  bool recreate_in_memory = false;

  // If tried to recreate database on disk already, try again but this time
  // in memory.
  if (tried_to_recreate_during_open_ && !partition_directory_path_.empty()) {
    recreate_in_memory = true;
  } else if (tried_to_recreate_during_open_) {
    // Give up completely, run without any database.
    OnConnectionFinished();
    return;
  }

  tried_to_recreate_during_open_ = true;

  // Unit tests might not have a bound file_service_, in which case there is
  // nothing to retry.
  if (!file_system_.is_bound()) {
    OnConnectionFinished();
    return;
  }

  // Destroy database, and try again.
  if (partition_directory_.is_bound()) {
    leveldb_service_->Destroy(
        std::move(partition_directory_), leveldb_name_,
        base::BindOnce(&SessionStorageContextMojo::OnDBDestroyed,
                       weak_ptr_factory_.GetWeakPtr(), recreate_in_memory));
  } else {
    // No directory, so nothing to destroy. Retrying to recreate will probably
    // fail, but try anyway.
    InitiateConnection(recreate_in_memory);
  }
}

void SessionStorageContextMojo::OnDBDestroyed(
    bool recreate_in_memory,
    leveldb::mojom::DatabaseError status) {
  UMA_HISTOGRAM_ENUMERATION("SessionStorageContext.DestroyDBResult",
                            leveldb::GetLevelDBStatusUMAValue(status),
                            leveldb_env::LEVELDB_STATUS_MAX);
  // We're essentially ignoring the status here. Even if destroying failed we
  // still want to go ahead and try to recreate.
  InitiateConnection(recreate_in_memory);
}

// The (possibly delayed) implementation of OpenSessionStorage(). Can be called
// directly from that function, or through |on_database_open_callbacks_|.
void SessionStorageContextMojo::BindSessionStorage(
    std::string namespace_id,
    const url::Origin& origin,
    mojom::LevelDBWrapperRequest request) {
  LOG(ERROR) << "got bind!";
  GetOrCreateDBWrapper(std::move(namespace_id), origin)->Bind(std::move(request));
}

SessionStorageContextMojo::LevelDBWrapperBinder*
SessionStorageContextMojo::GetOrCreateDBWrapper(const std::string& namespace_id,
                                                const url::Origin& origin) {
  DCHECK_EQ(connection_state_, CONNECTION_FINISHED);
  AreaKey area_key(namespace_id, origin);
  auto found = area_map_.find(area_key);
  if (found != area_map_.end()) {
    return found->second.get();
  }

  size_t total_cache_size, unused_wrapper_count;
  GetStatistics(&total_cache_size, &unused_wrapper_count);

  // Track the total sessionStorage cache size.
  UMA_HISTOGRAM_COUNTS_100000("SessionStorageContext.CacheSizeInKB",
                              total_cache_size / 1024);

  PurgeUnusedWrappersIfNeeded();

  std::vector<uint8_t> map_id = GetNextMapPrefix();
  auto data_map_it =
      data_maps_
          .emplace(map_id, std::make_unique<LevelDBRefCountedMap>(this, map_id))
          .first;

  LevelDBRefCountedMap* map_holder = data_map_it->second.get();
  auto map_it = area_map_
                    .insert(std::make_pair(
                        std::move(area_key),
                        std::make_unique<LevelDBWrapperBinder>(
                            this, std::move(namespace_id), map_holder)))
                    .first;
  return map_it->second.get();
}

void SessionStorageContextMojo::RetrieveStorageUsage(
    GetStorageUsageCallback callback) {
  if (!database_) {
    // If for whatever reason no leveldb database is available, no storage is
    // used, so return an array only containing the current leveldb wrappers.
    std::vector<SessionStorageUsageInfo> result;
    // XXX get storage usage
    std::move(callback).Run(std::move(result));
    return;
  }

  return;
  // XXX get storage usage.
}

void SessionStorageContextMojo::OnGotMetaData(
    GetStorageUsageCallback callback,
    leveldb::mojom::DatabaseError status,
    std::vector<leveldb::mojom::KeyValuePtr> data) {
  // xxx analyize and return storage usage.
}

void SessionStorageContextMojo::OnShutdownComplete(
    leveldb::mojom::DatabaseError error) {
  delete this;
}

void SessionStorageContextMojo::GetStatistics(size_t* total_cache_size,
                                              size_t* unused_wrapper_count) {
  *total_cache_size = 0;
  *unused_wrapper_count = 0;
  // XXX: analyze unused wrappers
}

void SessionStorageContextMojo::OnCommitResult(
    leveldb::mojom::DatabaseError error) {
  DCHECK_EQ(connection_state_, CONNECTION_FINISHED);
  if (error == leveldb::mojom::DatabaseError::OK) {
    commit_error_count_ = 0;
    return;
  }

  commit_error_count_++;
  if (commit_error_count_ > kCommitErrorThreshold) {
    if (tried_to_recover_from_commit_errors_) {
      // We already tried to recover from a high commit error rate before, but
      // are still having problems: there isn't really anything left to try, so
      // just ignore errors.
      return;
    }
    tried_to_recover_from_commit_errors_ = true;

    // Deleting LevelDBWrappers in here could cause more commits (and commit
    // errors), but those commits won't reach OnCommitResult because the wrapper
    // will have been deleted before the commit finishes.
    DeleteAndRecreateDatabase(
        "SessionStorageContext.OpenResultAfterCommitErrors");
  }
}

std::vector<uint8_t> SessionStorageContextMojo::GetNextMapPrefix() {
  return MapNumberToValue(next_map_id_++);
}

void SessionStorageContextMojo::LogDatabaseOpenResult(OpenResult result) {
  if (result != OpenResult::SUCCESS) {
    UMA_HISTOGRAM_ENUMERATION("SessionStorageContext.OpenError", result,
                              OpenResult::MAX);
  }
  if (open_result_histogram_) {
    base::UmaHistogramEnumeration(open_result_histogram_, result,
                                  OpenResult::MAX);
  }
}

std::string SessionStorageContextMojo::NamespaceStartKey(
    const std::string& namespace_id) {
  return base::StringPrintf("namespace-%s-", namespace_id.c_str());
}

std::string SessionStorageContextMojo::NamespaceKey(
    const std::string& namespace_id, const std::string& origin) {
  return base::StringPrintf("namespace-%s-%s", namespace_id.c_str(),
                            origin.c_str());
}

std::string SessionStorageContextMojo::MapRefCountKey(const std::string& map_id) {
  return base::StringPrintf("map-%s-", map_id.c_str());
}

std::string SessionStorageContextMojo::MapKey(const std::string& map_id,
                                           const std::string& key) {
  return base::StringPrintf("map-%s-%s", map_id.c_str(), key.c_str());
}

}  // namespace content
