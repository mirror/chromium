// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOM_STORAGE_SESSION_STORAGE_CONTEXT_MOJO_H_
#define CONTENT_BROWSER_DOM_STORAGE_SESSION_STORAGE_CONTEXT_MOJO_H_

#include <stdint.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/callback_forward.h"
#include "base/containers/small_map.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/trace_event/memory_dump_provider.h"
#include "content/common/content_export.h"
#include "content/common/leveldb_wrapper.mojom.h"
#include "services/file/public/interfaces/file_system.mojom.h"
#include "url/origin.h"

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace service_manager {
class Connector;
}  // namespace service_manager

namespace content {

struct SessionStorageUsageInfo;

// Used for mojo-based SessionStorage implementation.
// Created on the UI thread, but all further methods are called on the task
// runner passed to the constructor. Furthermore since destruction of this class
// can involve asynchronous steps, it can only be deleted by calling
// ShutdownAndDelete (on the correct task runner).
class CONTENT_EXPORT SessionStorageContextMojo
    : public base::trace_event::MemoryDumpProvider {
 public:
  using GetStorageUsageCallback =
      base::OnceCallback<void(std::vector<SessionStorageUsageInfo>)>;

  SessionStorageContextMojo(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      service_manager::Connector* connector,
      base::FilePath partition_directory,
      std::string leveldb_name);
  ~SessionStorageContextMojo() override;

  void OpenSessionStorage(int64_t namespace_id,
                          const url::Origin& origin,
                          mojom::LevelDBWrapperRequest request);

  // Creates a new namespace with the given ids. If there is data saved under
  // the |persistent_namespace_id| then this is now associated with the given
  // |namespace_id|.
  void CreateSessionNamespace(int64_t namespace_id,
                              std::string persistent_namespace_id);
  void CloneSessionNamespace(int64_t namespace_id_to_clone,
                             int64_t clone_namespace_id,
                             std::string clone_persistent_namespace_id);
  void DeleteSessionNamespace(int64_t namespace_id, bool should_persist);

  void Flush();
  void GetStorageUsage(GetStorageUsageCallback callback);
  void DeleteStorage(const GURL& origin,
                     const std::string& persistent_namespace_id);

  // Called when the owning BrowserContext is ending.
  // Schedules the commit of any unsaved changes and will delete
  // and keep data on disk per the content settings and special storage
  // policies.
  void ShutdownAndDelete();

  // Clears any caches, to free up as much memory as possible. Next access to
  // storage for a particular origin will reload the data from the database.
  void PurgeMemory();

  // Clears unused leveldb wrappers, when thresholds are reached.
  void PurgeUnusedWrappersIfNeeded();

  base::WeakPtr<SessionStorageContextMojo> AsWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

  // base::trace_event::MemoryDumpProvider implementation.
  bool OnMemoryDump(const base::trace_event::MemoryDumpArgs& args,
                    base::trace_event::ProcessMemoryDump* pmd) override;

 private:
  using MapId = std::vector<uint8_t>;

  class LevelDBWrapperBinder;
  class LevelDBRefCountedMap;

  // Runs |callback| immediately if already connected to a database, otherwise
  // delays running |callback| untill after a connection has been established.
  // Initiates connecting to the database if no connection is in progres yet.
  void RunWhenConnected(base::OnceClosure callback);

  // Part of our asynchronous directory opening called from RunWhenConnected().
  void InitiateConnection(bool in_memory_only = false);
  void OnDirectoryOpened(filesystem::mojom::FileError err);
  void OnDatabaseOpened(bool in_memory, leveldb::mojom::DatabaseError status);
  void OnGotDatabaseVersion(leveldb::mojom::DatabaseError status,
                            const std::vector<uint8_t>& value);
  void OnGotNamespaces(base::OnceClosure done,
                       leveldb::mojom::DatabaseError status,
                       std::vector<leveldb::mojom::KeyValuePtr> values);
  void OnGotNextMapId(base::OnceClosure done,
                      leveldb::mojom::DatabaseError status,
                      const std::vector<uint8_t>& map_id);
  void OnConnectionFinished();
  void DeleteAndRecreateDatabase(const char* histogram_name);
  void OnDBDestroyed(bool recreate_in_memory,
                     leveldb::mojom::DatabaseError status);

  // The (possibly delayed) implementation of OpenSessionStorage(). Can be
  // called directly from that function, or through
  // |on_database_open_callbacks_|.
  void BindSessionStorage(std::string namespace_id,
                          const url::Origin& origin,
                          mojom::LevelDBWrapperRequest request);
  LevelDBWrapperBinder* GetOrCreateDBWrapper(const std::string& namespace_id,
                                             const url::Origin& origin);


  // The (possibly delayed) implementation of GetStorageUsage(). Can be called
  // directly from that function, or through |on_database_open_callbacks_|.
  void RetrieveStorageUsage(GetStorageUsageCallback callback);
  void OnGotMetaData(GetStorageUsageCallback callback,
                     leveldb::mojom::DatabaseError status,
                     std::vector<leveldb::mojom::KeyValuePtr> data);

  void OnShutdownComplete(leveldb::mojom::DatabaseError error);

  void GetStatistics(size_t* total_cache_size, size_t* unused_wrapper_count);
  void OnCommitResult(leveldb::mojom::DatabaseError error);

  std::vector<uint8_t> GetNextMapPrefix();

  // These values are written to logs.  New enum values can be added, but
  // existing enums must never be renumbered or deleted and reused.
  enum class OpenResult {
    DIRECTORY_OPEN_FAILED = 0,
    DATABASE_OPEN_FAILED = 1,
    INVALID_VERSION = 2,
    VERSION_READ_ERROR = 3,
    NAMESPACES_READ_ERROR = 4,
    NEXT_MAP_ID_READ_ERROR = 5,
    SUCCESS = 6,
    MAX
  };

  void LogDatabaseOpenResult(OpenResult result);

  // Helper functions for creating the keys needed for the schema.
  static std::string NamespaceStartKey(const std::string& namespace_id);
  static std::string NamespaceKey(const std::string& namespace_id,
                                  const std::string& origin);
  static std::string MapRefCountKey(const std::string& map_id);
  static std::string MapKey(const std::string& map_id, const std::string& key);

  std::map<MapId, std::unique_ptr<LevelDBRefCountedMap>> data_maps_;

  // A namespace and an origin uniquely identifies a session storage map.

  std::unique_ptr<service_manager::Connector> connector_;
  const base::FilePath partition_directory_path_;
  std::string leveldb_name_;

  enum ConnectionState {
    NO_CONNECTION,
    FETCHING_METADATA,
    CONNECTION_IN_PROGRESS,
    CONNECTION_FINISHED,
    CONNECTION_SHUTDOWN
  } connection_state_ = NO_CONNECTION;
  bool database_initialized_ = false;
  int64_t next_map_id_ = 0;
  // Used as a backup id if reading the next id fails.
  int64_t next_map_id_from_namespaces_ = 0;

  file::mojom::FileSystemPtr file_system_;
  filesystem::mojom::DirectoryPtr partition_directory_;

  base::trace_event::MemoryAllocatorDumpGuid memory_dump_id_;

  leveldb::mojom::LevelDBServicePtr leveldb_service_;
  leveldb::mojom::LevelDBDatabaseAssociatedPtr database_;
  bool tried_to_recreate_during_open_ = false;

  std::vector<base::OnceClosure> on_database_opened_callbacks_;

  // Namespace id to persistent id.
  std::unordered_map<int64_t, std::string> namespaces_;

  // This map contains all session storage metadata.
  using AreaKey = std::pair<std::string, url::Origin>;
  using SessionStorageMap =
      std::map<AreaKey, std::unique_ptr<LevelDBWrapperBinder>>;
  SessionStorageMap area_map_;

  bool is_low_end_device_;
  // Counts consecutive commit errors. If this number reaches a threshold, the
  // whole database is thrown away.
  int commit_error_count_ = 0;
  bool tried_to_recover_from_commit_errors_ = false;

  // Name of an extra histogram to log open results to, if not null.
  const char* open_result_histogram_ = nullptr;

  base::WeakPtrFactory<SessionStorageContextMojo> weak_ptr_factory_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOM_STORAGE_SESSION_STORAGE_CONTEXT_MOJO_H_
