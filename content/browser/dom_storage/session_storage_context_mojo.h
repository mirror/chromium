// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOM_STORAGE_SESSION_STORAGE_CONTEXT_MOJO_H_
#define CONTENT_BROWSER_DOM_STORAGE_SESSION_STORAGE_CONTEXT_MOJO_H_

#include <memory>

#include "content/common/leveldb_wrapper.mojom.h"
#include "services/file/public/interfaces/file_system.mojom.h"
#include "url/origin.h"

namespace service_manager {
class Connection;
class Connector;
}

namespace content {
class LevelDBWrapperImpl;

// Used for mojo-based SessionStorage implementation.
class CONTENT_EXPORT SessionStorageContextMojo {
 public:
  SessionStorageContextMojo(service_manager::Connector* connector,
                            const base::FilePath& subdirectory);
  ~SessionStorageContextMojo();

  void OpenSessionStorage(int64_t namespace_id,
                          const url::Origin& origin,
                          mojom::LevelDBWrapperRequest request);
  void OpenSessionStorage(const std::string& persistent_namespace_id,
                          const url::Origin& origin,
                          mojom::LevelDBWrapperRequest request);

  void CreateSessionNamespace(int64_t namespace_id,
                              const std::string& persistent_namespace_id);
  void CloneSessionNamespace(int64_t namespace_id_to_clone,
                             int64_t clone_namespace_id,
                             const std::string& clone_persistent_namespace_id);
  void DeleteSessionNamespace(int64_t namespace_id, bool should_persist);

 private:
  struct Namespace;

  // Runs |callback| immediately if already connected to a database, otherwise
  // delays running |callback| untill after a connection has been established.
  // Initiates connecting to the database if no connection is in progres yet.
  void RunWhenConnected(base::OnceClosure callback);
  void InitiateConnection(bool in_memory_only = false);
  void OnDirectoryOpened(filesystem::mojom::FileError err);
  void OnDatabaseOpened(leveldb::mojom::DatabaseError status);

  void OnUserServiceConnectionComplete();
  void OnUserServiceConnectionError();

  // The (possibly delayed) implementation of OpenSessionStorage().
  void BindSessionStorage(const std::string& namespace_id,
                          const url::Origin& origin,
                          mojom::LevelDBWrapperRequest request);

  service_manager::Connector* const connector_;
  const base::FilePath subdirectory_;

  enum ConnectionState {
    NO_CONNECTION,
    CONNECTION_IN_PROGRESS,
    CONNECTION_FINISHED
  } connection_state_ = NO_CONNECTION;

  std::unique_ptr<service_manager::Connection> file_service_connection_;

  file::mojom::FileSystemPtr file_system_;
  filesystem::mojom::DirectoryPtr directory_;

  leveldb::mojom::LevelDBServicePtr leveldb_service_;
  leveldb::mojom::LevelDBDatabaseAssociatedPtr database_;

  std::vector<base::OnceClosure> on_database_opened_callbacks_;

  // Store in memory:
  // next-map-id
  // map-id (int64_t) -> refcount
  // namespace+origin -> (map-id, LevelDBWrapperImpl)
  struct Map;
  std::map<int64_t, Map> maps_;
  using AreaKey = std::pair<std::string, url::Origin>;
  class Area;
  using AreaMap = std::map<AreaKey, std::unique_ptr<Area>>;
  AreaMap areas_;

  std::map<int64_t, std::string> namespaces_;

  base::WeakPtrFactory<SessionStorageContextMojo> weak_ptr_factory_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOM_STORAGE_SESSION_STORAGE_CONTEXT_MOJO_H_
