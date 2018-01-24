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
#include "base/debug/stack_trace.h"
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
// constexpr const char kNamespaceOriginSeperator = '-';
constexpr const size_t kNamespaceOriginSeperatorLength = 1;
constexpr const size_t kPrefixBeforeOriginLength =
    kNamespacePrefixLength + kPersistentSessionIdLength +
    kNamespaceOriginSeperatorLength;

const char kSessionStorageVersionKey[] = "VERSION";
constexpr const char kNextMapIdKey[] = "next-map-id";
const int64_t kMinSessionStorageSchemaVersion = 1;
const int64_t kCurrentSessionStorageSchemaVersion = 1;
// After this many consecutive commit errors we'll throw away the entire
// database.
const int kCommitErrorThreshold = 8;

// Limits on the cache size and number of areas in memory, over which the areas
// are purged.
#if defined(OS_ANDROID)
const unsigned kMaxSessionStorageAreaCount = 10;
const size_t kMaxSessionStorageCacheSize = 2 * 1024 * 1024;
#else
const unsigned kMaxSessionStorageAreaCount = 50;
const size_t kMaxSessionStorageCacheSize = 20 * 1024 * 1024;
#endif

}  // namespace

enum class CachePurgeReason {
  NotNeeded,
  SizeLimitExceeded,
  AreaCountLimitExceeded,
  InactiveOnLowEndDevice,
  AggressivePurgeTriggered
};

void RecordCachePurgedHistogram(CachePurgeReason reason,
                                size_t purged_size_kib) {
  UMA_HISTOGRAM_COUNTS_100000("SessionStorageContext.CachePurgedInKB",
                              purged_size_kib);
  switch (reason) {
    case CachePurgeReason::SizeLimitExceeded:
      UMA_HISTOGRAM_COUNTS_100000(
          "SessionStorageContext.CachePurgedInKB.SizeLimitExceeded",
          purged_size_kib);
      break;
    case CachePurgeReason::AreaCountLimitExceeded:
      UMA_HISTOGRAM_COUNTS_100000(
          "SessionStorageContext.CachePurgedInKB.AreaCountLimitExceeded",
          purged_size_kib);
      break;
    case CachePurgeReason::InactiveOnLowEndDevice:
      UMA_HISTOGRAM_COUNTS_100000(
          "SessionStorageContext.CachePurgedInKB.InactiveOnLowEndDevice",
          purged_size_kib);
      break;
    case CachePurgeReason::AggressivePurgeTriggered:
      UMA_HISTOGRAM_COUNTS_100000(
          "SessionStorageContext.CachePurgedInKB.AggressivePurgeTriggered",
          purged_size_kib);
      break;
    case CachePurgeReason::NotNeeded:
      NOTREACHED();
      break;
  }
}

void NoOpSuccess(bool success) {}
void NoOpLevelDB(leveldb::mojom::DatabaseError) {}

bool ValueToMapNumber(const std::vector<uint8_t>& value, int64_t* out) {
  const base::StringPiece piece(reinterpret_cast<const char*>(&(value[0])),
                                value.size());
  return base::StringToInt64(piece, out);
}

std::vector<uint8_t> MapNumberToValue(int64_t map_number) {
  return leveldb::StdStringToUint8Vector(base::NumberToString(map_number));
}

SessionStorageContextMojo::SessionStorageContextMojo(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    service_manager::Connector* connector,
    base::Optional<base::FilePath> partition_directory,
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
  LOG(INFO) << "OpenSessionStorage " << namespace_id;
  DCHECK(namespaces_.find(namespace_id) != namespaces_.end());
  std::string namespace_str_id = namespaces_[namespace_id];

  AreaKey area_key(namespace_str_id, origin);
  auto found = area_map_.find(area_key);
  if (found != area_map_.end()) {
    found->second->Bind(std::move(request));
    if (connection_state_ == CONNECTION_FINISHED) {
      found->second->StartOperations();
    } else {
      InitiateConnectionIfDisconnected();
      on_database_opened_callbacks_.push_back(
          found->second->CreateStartClosure());
    }
    return;
  }

  auto map_it = area_map_
                    .insert(std::make_pair(
                        std::move(area_key),
                        std::make_unique<SessionStorageLevelDBWrapper>(this)))
                    .first;

  if (connection_state_ == CONNECTION_FINISHED) {
    size_t total_cache_size, unused_wrapper_count;
    GetStatistics(&total_cache_size, &unused_wrapper_count);

    // Track the total sessionStorage cache size.
    UMA_HISTOGRAM_COUNTS_100000("SessionStorageContext.CacheSizeInKB",
                                total_cache_size / 1024);

    PurgeUnusedWrappersIfNeeded();
    map_it->second->OnDBConnectionComplete(database_.get(),
                                           GetAndWriteNextMapPrefix());
    map_it->second->StartOperations();
  } else {
    InitiateConnectionIfDisconnected();
    on_database_opened_callbacks_.push_back(
        map_it->second->CreateStartClosure());
  }

  map_it->second->Bind(std::move(request));
}

void SessionStorageContextMojo::CreateSessionNamespace(
    int64_t namespace_id,
    std::string persistent_namespace_id) {
  LOG(INFO) << "CreateSessionNamespace " << namespace_id << " with p "
            << persistent_namespace_id;
  DCHECK(namespaces_.find(namespace_id) == namespaces_.end());
  namespaces_[namespace_id] = std::move(persistent_namespace_id);
}

void SessionStorageContextMojo::CloneSessionNamespace(
    int64_t namespace_id_to_clone,
    int64_t clone_namespace_id,
    std::string clone_persistent_namespace_id) {
  LOG(INFO) << "CloneSessionNamespace " << namespace_id_to_clone << " to "
            << clone_namespace_id;
  DCHECK(namespaces_.find(namespace_id_to_clone) != namespaces_.end());
  DCHECK(namespaces_.find(clone_namespace_id) == namespaces_.end());

  namespaces_[clone_namespace_id] = clone_persistent_namespace_id;

  const std::string& namespace_pid = namespaces_[namespace_id_to_clone];
  auto map_it = area_map_.lower_bound(
      std::pair<std::string, url::Origin>(namespace_pid, url::Origin()));

  std::vector<SessionStorageMap::value_type> to_insert;
  for (; map_it != area_map_.end() && map_it->first.first == namespace_pid;
       map_it++) {
    LOG(INFO) << "cloning " << map_it->first.first;
    to_insert.push_back(std::make_pair(
        std::make_pair(clone_persistent_namespace_id, map_it->first.second),
        map_it->second->Clone()));
  }
  using insert_iterator = std::vector<SessionStorageMap::value_type>::iterator;
  area_map_.insert(std::move_iterator<insert_iterator>(to_insert.begin()),
                   std::move_iterator<insert_iterator>(to_insert.end()));
  DCHECK(area_map_.lower_bound(std::pair<std::string, url::Origin>(
             clone_persistent_namespace_id, url::Origin())) != area_map_.end());
}

void SessionStorageContextMojo::DeleteSessionNamespace(int64_t namespace_id,
                                                       bool should_persist) {
  LOG(INFO) << "DeleteSessionNamespace " << namespace_id << " with "
            << should_persist;
  // xxx
}

void SessionStorageContextMojo::Flush() {
  if (connection_state_ != CONNECTION_FINISHED) {
    RunWhenConnected(base::BindOnce(&SessionStorageContextMojo::Flush,
                                    weak_ptr_factory_.GetWeakPtr()));
    return;
  }
  for (const auto& it : data_maps_)
    it.second->level_db_wrapper()->ScheduleImmediateCommit();
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
  LOG(INFO) << "DeleteStorage " << origin.spec() << " and "
            << persistent_namespace_id;

  AreaKey key(persistent_namespace_id, url::Origin::Create(origin));
  auto found = area_map_.find(key);
  if (found != area_map_.end()) {
    // Renderer process expects |source| to always be two newline separated
    // strings.
    found->second->DeleteAll("\n", base::BindOnce(&NoOpSuccess));
    found->second->ref_counted_map()
        ->level_db_wrapper()
        ->ScheduleImmediateCommit();
  }
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

  // Flush any uncommitted data.
  for (const auto& it : data_maps_) {
    auto* wrapper = it.second->level_db_wrapper();
    LOCAL_HISTOGRAM_BOOLEAN(
        "SessionStorageContext.ShutdownAndDelete.MaybeDroppedChanges",
        wrapper->has_pending_load_tasks());
    wrapper->ScheduleImmediateCommit();
    // TODO(dmurph): Monitor the above histogram, and if dropping changes is
    // common then handle that here.
    wrapper->CancelAllPendingRequests();
  }

  if (force_keep_session_state_) {
    OnShutdownComplete(leveldb::mojom::DatabaseError::OK);
    return;  // Keep everything.
  }

  // Delete everything.
  // xxx

  if (!partition_directory_path_) {
    LOG(INFO) << "partition directory path is empty";
    database_.reset();
    OnShutdownComplete(leveldb::mojom::DatabaseError::OK);
    return;
  }

  // The object should be deleted on OnShutdownComplete, so the directory
  // binding is no longer needed.
  leveldb_service_->Destroy(std::move(partition_directory_), leveldb_name_,
                            base::BindOnce(&NoOpLevelDB));
  OnShutdownComplete(leveldb::mojom::DatabaseError::OK);
}

void SessionStorageContextMojo::PurgeMemory() {
  size_t total_cache_size, unused_wrapper_count;
  GetStatistics(&total_cache_size, &unused_wrapper_count);

  // Purge all wrappers that don't have bindings.
  for (auto it = area_map_.begin(); it != area_map_.end();) {
    if (!it->second->IsBound())
      it = area_map_.erase(it);
  }

  // Track the size of cache purged.
  size_t final_total_cache_size;
  GetStatistics(&final_total_cache_size, &unused_wrapper_count);
  size_t purged_size_kib = (total_cache_size - final_total_cache_size) / 1024;
  RecordCachePurgedHistogram(CachePurgeReason::AggressivePurgeTriggered,
                             purged_size_kib);
}

void SessionStorageContextMojo::PurgeUnusedWrappersIfNeeded() {
  size_t total_cache_size, unused_wrapper_count;
  GetStatistics(&total_cache_size, &unused_wrapper_count);

  // Nothing to purge.
  if (!unused_wrapper_count)
    return;

  CachePurgeReason purge_reason = CachePurgeReason::NotNeeded;

  if (total_cache_size > kMaxSessionStorageCacheSize)
    purge_reason = CachePurgeReason::SizeLimitExceeded;
  else if (data_maps_.size() > kMaxSessionStorageAreaCount)
    purge_reason = CachePurgeReason::AreaCountLimitExceeded;
  else if (is_low_end_device_)
    purge_reason = CachePurgeReason::InactiveOnLowEndDevice;

  if (purge_reason == CachePurgeReason::NotNeeded)
    return;

  // Purge all wrappers that don't have bindings.
  for (auto it = area_map_.begin(); it != area_map_.end();) {
    if (!it->second->IsBound())
      it = area_map_.erase(it);
  }
}

bool SessionStorageContextMojo::OnMemoryDump(
    const base::trace_event::MemoryDumpArgs& args,
    base::trace_event::ProcessMemoryDump* pmd) {
  // xxx
  return true;
}

void SessionStorageContextMojo::OnDataMapDestruction(
    const std::vector<uint8_t>& map_prefix) {
  data_maps_.erase(map_prefix);
}

void SessionStorageContextMojo::InitiateConnectionIfDisconnected() {
  if (connection_state_ == NO_CONNECTION) {
    connection_state_ = CONNECTION_IN_PROGRESS;
    InitiateConnection();
  }
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
  LOG(INFO) << "initiating connection";
  // Unit tests might not always have a Connector, use in-memory only if that
  // happens.
  if (!connector_) {
    OnDatabaseOpened(false, leveldb::mojom::DatabaseError::OK);
    return;
  }

  if (!!partition_directory_path_ && !in_memory_only) {
    LOG(INFO) << "have a path";
    // We were given a subdirectory to write to. Get it and use a disk backed
    // database.
    connector_->BindInterface(file::mojom::kServiceName, &file_system_);
    file_system_->GetSubDirectory(
        partition_directory_path_.value().AsUTF8Unsafe(),
        MakeRequest(&partition_directory_),
        base::BindOnce(&SessionStorageContextMojo::OnDirectoryOpened,
                       weak_ptr_factory_.GetWeakPtr()));
  } else {
    LOG(INFO) << "in-memory";
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
  LOG(INFO) << "directory opened";
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
  LOG(INFO) << "OnDatabaseOpened";
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
    LOG(INFO) << "getting version";
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
    LOG(INFO) << "no db found";
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
    LOG(INFO) << "db error";
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
  database_->Get(leveldb::StdStringToUint8Vector(kNextMapIdKey),
                 base::BindOnce(&SessionStorageContextMojo::OnGotNextMapId,
                                weak_ptr_factory_.GetWeakPtr(), barrier));
}

void SessionStorageContextMojo::OnGotNamespaces(
    base::OnceClosure done,
    leveldb::mojom::DatabaseError status,
    std::vector<leveldb::mojom::KeyValuePtr> values) {
  if (connection_state_ != FETCHING_METADATA)
    return;
  if (status == leveldb::mojom::DatabaseError::NOT_FOUND) {
    LOG(INFO) << "no namespaces found!";
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

  // We buffer everything in a map so that we can analyze pre-bound areas
  // first. This prevents a map from being creates as an unbound area before it
  // is needed in a previously bound area (which already has a map iterated and
  // possibly shared).

  LOG(INFO) << "analyzing " << values.size() << " namespace values";

  std::map<AreaKey, std::vector<uint8_t>> areas_to_map_ids;

  for (leveldb::mojom::KeyValuePtr& key_value : values) {
    std::vector<uint8_t>& map_id = key_value->value;
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
      areas_to_map_ids.insert(std::make_pair(
          AreaKey(namespace_id, url::Origin::Create(GURL(origin_str))),
          std::move(map_id)));
    }
  }

  // The pre-bound areas need to be populated first to guarentee that any
  // copies done with these areas share the same map object.
  LOG(INFO) << "populating " << area_map_.size() << " prebound maps";
  for (auto& area_wrapper_pair : area_map_) {
    if (area_wrapper_pair.second->ref_counted_map()->IsConnected()) {
      LOG(INFO) << "map already populated!";
      continue;
    }

    const AreaKey& area_key = area_wrapper_pair.first;
    auto map_id_it = areas_to_map_ids.find(area_key);
    std::vector<uint8_t> map_id;
    if (map_id_it == areas_to_map_ids.end()) {
      LOG(INFO) << "new id!";
      map_id = GetAndWriteNextMapPrefix();
    } else {
      LOG(INFO) << "id in database!";
      map_id = map_id_it->second;
    }

    SessionStorageLevelDBWrapper* wrapper = area_wrapper_pair.second.get();
    auto data_map_it = data_maps_.find(map_id);
    if (data_map_it == data_maps_.end()) {
      DCHECK(!wrapper->ref_counted_map()->IsConnected());
      data_maps_.insert(std::make_pair(map_id, wrapper->ref_counted_map()));
      wrapper->OnDBConnectionComplete(database_.get(), std::move(map_id));

      int64_t map_number;
      if (ValueToMapNumber(map_id, &map_number) &&
          map_number >= next_map_id_from_namespaces_) {
        next_map_id_from_namespaces_ = map_number + 1;
        if (next_map_id_ == 0)
          next_map_id_ = next_map_id_from_namespaces_;
      }
    }
  }

  // Finally construct the rest of the areas that were not already bound.
  for (auto& area_map_id_pair : areas_to_map_ids) {
    const AreaKey& area_key = area_map_id_pair.first;
    auto area_map_it = area_map_.find(area_key);
    if (area_map_it != area_map_.end())
      continue;

    const std::vector<uint8_t>& map_id = area_map_id_pair.second;

    std::unique_ptr<SessionStorageLevelDBWrapper> wrapper;
    auto data_map_it = data_maps_.find(map_id);
    if (data_map_it == data_maps_.end()) {
      wrapper = std::make_unique<SessionStorageLevelDBWrapper>(this);
      wrapper->OnDBConnectionComplete(database_.get(), map_id);
      data_maps_.insert(std::make_pair(map_id, wrapper->ref_counted_map()));

      int64_t map_number;
      if (ValueToMapNumber(map_id, &map_number) &&
          map_number >= next_map_id_from_namespaces_) {
        next_map_id_from_namespaces_ = map_number + 1;
        if (next_map_id_ == 0)
          next_map_id_ = next_map_id_from_namespaces_;
      }
    } else {
      wrapper = std::make_unique<SessionStorageLevelDBWrapper>(
          this, base::WrapRefCounted(data_map_it->second));
    }

    area_map_.insert(std::make_pair(std::move(area_key), std::move(wrapper)));
  }

  std::move(done).Run();
}
void SessionStorageContextMojo::OnGotNextMapId(
    base::OnceClosure done,
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
    if (next_map_id_ < next_map_id_from_namespaces_)
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
  LOG(INFO) << "OnConnectionFinished";
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
  for (const auto& it : data_maps_)
    it.second->level_db_wrapper()->CancelAllPendingRequests();
  area_map_.clear();
  DCHECK(data_maps_.empty());

  // Reset state to be in process of connecting. This will cause requests for
  // LevelDBWrappers to be queued until the connection is complete.
  connection_state_ = CONNECTION_IN_PROGRESS;
  commit_error_count_ = 0;
  database_ = nullptr;
  open_result_histogram_ = histogram_name;

  bool recreate_in_memory = false;

  // If tried to recreate database on disk already, try again but this time
  // in memory.
  if (tried_to_recreate_during_open_ && !!partition_directory_path_) {
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

void SessionStorageContextMojo::RetrieveStorageUsage(
    GetStorageUsageCallback callback) {
  // If for whatever reason no leveldb database is available, no storage is
  // used, so return an array only containing the current leveldb wrappers.
  std::vector<SessionStorageUsageInfo> result;
  result.reserve(area_map_.size());
  for (const auto& pair : area_map_) {
    SessionStorageUsageInfo info = {pair.first.second.GetURL(),
                                    pair.first.first};
    result.push_back(std::move(info));
  }
  std::move(callback).Run(std::move(result));
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
  for (const auto& it : data_maps_) {
    *total_cache_size += it.second->level_db_wrapper()->memory_used();
    if (it.second->binding_count() == 0)
      (*unused_wrapper_count)++;
  }
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

std::vector<uint8_t> SessionStorageContextMojo::GetAndWriteNextMapPrefix() {
  std::vector<uint8_t> current_id = MapNumberToValue(next_map_id_);
  ++next_map_id_;
  database_->Put(leveldb::StdStringToUint8Vector(kNextMapIdKey),
                 MapNumberToValue(next_map_id_), base::BindOnce(&NoOpLevelDB));
  return current_id;
}

void SessionStorageContextMojo::LogDatabaseOpenResult(OpenResult result) {
  if (result != OpenResult::SUCCESS) {
    LOG(ERROR) << "Got error when openning: " << static_cast<int>(result);
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
    const std::string& namespace_id,
    const std::string& origin) {
  return base::StringPrintf("namespace-%s-%s", namespace_id.c_str(),
                            origin.c_str());
}

std::string SessionStorageContextMojo::MapKey(const std::string& map_id,
                                              const std::string& key) {
  return base::StringPrintf("map-%s-%s", map_id.c_str(), key.c_str());
}

}  // namespace content
