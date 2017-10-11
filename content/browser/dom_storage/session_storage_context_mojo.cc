// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/dom_storage/session_storage_context_mojo.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "components/leveldb/public/cpp/util.h"
#include "content/browser/leveldb_wrapper_impl.h"
#include "content/common/dom_storage/dom_storage_types.h"
#include "services/file/public/interfaces/constants.mojom.h"
#include "services/service_manager/public/cpp/connection.h"
#include "services/service_manager/public/cpp/connector.h"

namespace content {

namespace {

void NoOp() {}

void BindRequest(mojom::LevelDBWrapperRequest request, LevelDBWrapperImpl* wrapper) {
  wrapper->Bind(std::move(request));
}

// Delay for a moment after a value is set in anticipation
// of other values being set, so changes are batched.
const int kCommitDefaultDelaySecs = 5;

// To avoid excessive IO we apply limits to the amount of data being written
// and the frequency of writes.
const int kMaxBytesPerHour = kPerStorageAreaQuota;
const int kMaxCommitsPerHour = 60;

}  // namespace

class SessionStorageContextMojo::Area : public LevelDBWrapperImpl::Delegate {
 public:
  using ReadyCallback = base::OnceCallback<void(LevelDBWrapperImpl*)>;

  Area(SessionStorageContextMojo* context,
       const std::string& namespace_id,
       const url::Origin& origin) : context_(context), namespace_id_(namespace_id), origin_(origin), weak_ptr_factory_(this) {
    if (!context_->database_) {
      // If operating without a database, immediately consider ourselves ready.
      OnReady();
      return;
    }

    context_->database_->Get(namespace_key(), base::Bind(&Area::OnGotMapId, weak_ptr_factory_.GetWeakPtr()));
  }

  std::vector<uint8_t> namespace_key() const {
    return leveldb::StdStringToUint8Vector(base::StringPrintf("namespace-%s-%s", namespace_id_.c_str(), origin_.Serialize().c_str()));
  }

  std::string map_prefix() const {
    return base::StringPrintf("map-%" PRId64 "-", map_id_);
  }

  void CallWhenReady(ReadyCallback callback) {
    if (ready_) {
      std::move(callback).Run(level_db_wrapper());
      return;
    }
    ready_callbacks_.push_back(std::move(callback));
  }

  LevelDBWrapperImpl* level_db_wrapper() { return level_db_wrapper_ptr_; }

  void OnNoBindings() override {
  }

  std::vector<leveldb::mojom::BatchedOperationPtr> PrepareToCommit() override {
    std::vector<leveldb::mojom::BatchedOperationPtr> operations;
    (void) context_;
    (void) map_id_;
    // set map_id_ if not set already, clone if refcount > 1
    return operations;
  }

  void DidCommit(leveldb::mojom::DatabaseError error) override {
  }

 private:
  void OnReady() {
    level_db_wrapper_ = base::MakeUnique<LevelDBWrapperImpl>(context_->database_.get(), map_prefix(), kPerStorageAreaQuota + kPerStorageAreaOverQuotaAllowance, base::TimeDelta::FromSeconds(kCommitDefaultDelaySecs), kMaxBytesPerHour,
        kMaxCommitsPerHour, this);
    level_db_wrapper_ptr_ = level_db_wrapper_.get();
    ready_ = true;
    for (auto& callback: ready_callbacks_)
      std::move(callback).Run(level_db_wrapper());
    ready_callbacks_.clear();
  }

  void OnGotMapId(leveldb::mojom::DatabaseError status,
                  const std::vector<uint8_t>& value) {
    if (status == leveldb::mojom::DatabaseError::NOT_FOUND) {
      // Map does not exist yet, will be created when data gets committed.
    } else if (status != leveldb::mojom::DatabaseError::OK) {
      // some problem...
    } else {
      if (base::StringToInt64(leveldb::Uint8VectorToStdString(value), &map_id_)) {
        // blah, this sucks
      }
      exists_ = true;
    }
    OnReady();
  }


  SessionStorageContextMojo* context_;

  std::string namespace_id_;
  url::Origin origin_;
  int64_t map_id_ = 0;
  bool exists_ = false;
  std::unique_ptr<LevelDBWrapperImpl> level_db_wrapper_;
  // Holds the same value as |level_db_wrapper_|. The reason for this is that
  // during destruction of the LevelDBWrapperImpl instance we might still get
  // called and need access  to the LevelDBWrapperImpl instance. The unique_ptr
  // could already be null, but this field should still be valid.
  LevelDBWrapperImpl* level_db_wrapper_ptr_;

  bool ready_ = false;
  std::vector<ReadyCallback> ready_callbacks_;

  base::WeakPtrFactory<Area> weak_ptr_factory_;
};

struct SessionStorageContextMojo::Map {
  int64_t id;
  int refcount = 0;
};

SessionStorageContextMojo::SessionStorageContextMojo(
    service_manager::Connector* connector,
    const base::FilePath& subdirectory)
    : connector_(connector),
      subdirectory_(subdirectory),
      weak_ptr_factory_(this) {}

SessionStorageContextMojo::~SessionStorageContextMojo() {}

void SessionStorageContextMojo::OpenSessionStorage(int64_t namespace_id,
                                                   const url::Origin& origin,
                                                   mojom::LevelDBWrapperRequest request) {
  auto it = namespaces_.find(namespace_id);
  if (it == namespaces_.end()) {
    LOG(INFO) << "Invalid namespace id, should kill renderer";
    return;
  }
  OpenSessionStorage(it->second, origin, std::move(request));
}

void SessionStorageContextMojo::OpenSessionStorage(const std::string& persistent_namespace_id,
                                                   const url::Origin& origin,
                                                   mojom::LevelDBWrapperRequest request) {
  RunWhenConnected(base::BindOnce(&SessionStorageContextMojo::BindSessionStorage,
    base::Unretained(this), persistent_namespace_id, origin, std::move(request)));
}

void SessionStorageContextMojo::CreateSessionNamespace(int64_t namespace_id,
                                                       const std::string& persistent_namespace_id) {
  DCHECK_NE(namespace_id, kLocalStorageNamespaceId);
  DCHECK(namespaces_.find(namespace_id) == namespaces_.end());
  namespaces_[namespace_id] = persistent_namespace_id;
}

void SessionStorageContextMojo::CloneSessionNamespace(int64_t namespace_id_to_clone,
                             int64_t clone_namespace_id,
                             const std::string& clone_persistent_namespace_id) {
  DCHECK_NE(namespace_id_to_clone, kLocalStorageNamespaceId);
  DCHECK_NE(clone_namespace_id, kLocalStorageNamespaceId);
  auto it = namespaces_.find(namespace_id_to_clone);
  if (it == namespaces_.end()) {
    CreateSessionNamespace(clone_namespace_id, clone_persistent_namespace_id);
    return;
  }

  // TODO
  LOG(INFO) << "Should clone namespace";
  // Shallow copy stuff for this namespace...
  namespaces_[clone_namespace_id] = clone_persistent_namespace_id;
}

void SessionStorageContextMojo::DeleteSessionNamespace(int64_t namespace_id, bool should_persist) {
  DCHECK_NE(namespace_id, kLocalStorageNamespaceId);
  namespaces_.erase(namespace_id);
}

void SessionStorageContextMojo::RunWhenConnected(base::OnceClosure callback) {
  // If we don't have a filesystem_connection_, we'll need to establish one.
  if (connection_state_ == NO_CONNECTION) {
    CHECK(connector_);
    file_service_connection_ = connector_->Connect(file::mojom::kServiceName);
    connection_state_ = CONNECTION_IN_PROGRESS;
    file_service_connection_->AddConnectionCompletedClosure(
        base::Bind(&SessionStorageContextMojo::OnUserServiceConnectionComplete,
                   weak_ptr_factory_.GetWeakPtr()));
    file_service_connection_->SetConnectionLostClosure(
        base::Bind(&SessionStorageContextMojo::OnUserServiceConnectionError,
                   weak_ptr_factory_.GetWeakPtr()));

    InitiateConnection();
  }

  if (connection_state_ == CONNECTION_IN_PROGRESS) {
    // Queue this OpenLocalStorage call for when we have a level db pointer.
    on_database_opened_callbacks_.push_back(std::move(callback));
    return;
  }

  std::move(callback).Run();
}

void SessionStorageContextMojo::InitiateConnection(bool in_memory_only) {
  DCHECK_EQ(connection_state_, CONNECTION_IN_PROGRESS);
  if (!subdirectory_.empty() && !in_memory_only) {
    // We were given a subdirectory to write to. Get it and use a disk backed
    // database.
    file_service_connection_->GetInterface(&file_system_);
    file_system_->GetSubDirectory(
        subdirectory_.AsUTF8Unsafe(), MakeRequest(&directory_),
        base::Bind(&SessionStorageContextMojo::OnDirectoryOpened,
                   weak_ptr_factory_.GetWeakPtr()));
  } else {
    // We were not given a subdirectory. Use a memory backed database.
    file_service_connection_->GetInterface(&leveldb_service_);
    leveldb_service_->OpenInMemory(
        MakeRequest(&database_, leveldb_service_.associated_group()),
        base::Bind(&SessionStorageContextMojo::OnDatabaseOpened,
                   weak_ptr_factory_.GetWeakPtr()));
  }
}

void SessionStorageContextMojo::OnDirectoryOpened(
    filesystem::mojom::FileError err) {
  if (err != filesystem::mojom::FileError::OK) {
    // We failed to open the directory; continue with startup so that we create
    // the |level_db_wrappers_|.
    OnDatabaseOpened(leveldb::mojom::DatabaseError::IO_ERROR);
    return;
  }

  // Now that we have a directory, connect to the LevelDB service and get our
  // database.
  file_service_connection_->GetInterface(&leveldb_service_);

  // We might still need to use the directory, so create a clone.
  filesystem::mojom::DirectoryPtr directory_clone;
  directory_->Clone(MakeRequest(&directory_clone));

  auto options = leveldb::mojom::OpenOptions::New();
  options->create_if_missing = true;
  leveldb_service_->OpenWithOptions(
      std::move(options), std::move(directory_clone), "leveldb",
      MakeRequest(&database_, leveldb_service_.associated_group()),
      base::Bind(&SessionStorageContextMojo::OnDatabaseOpened,
                 weak_ptr_factory_.GetWeakPtr()));
}

void SessionStorageContextMojo::OnDatabaseOpened(
    leveldb::mojom::DatabaseError status) {
  if (status != leveldb::mojom::DatabaseError::OK) {
    // If we failed to open the database, reset the service object so we pass
    // null pointers to our wrappers.
    database_.reset();
  }

  // We no longer need the file service; we've either transferred |directory_|
  // to the leveldb service, or we got a file error and no more is possible.
  directory_.reset();
  file_system_.reset();
  if (!database_)
    leveldb_service_.reset();

  // |database_| should be known to either be valid or invalid by now. Run our
  // delayed bindings.
  connection_state_ = CONNECTION_FINISHED;
  for (size_t i = 0; i < on_database_opened_callbacks_.size(); ++i)
    std::move(on_database_opened_callbacks_[i]).Run();
  on_database_opened_callbacks_.clear();
}

void SessionStorageContextMojo::OnUserServiceConnectionComplete() {
  CHECK_EQ(service_manager::mojom::ConnectResult::SUCCEEDED,
           file_service_connection_->GetResult());
}

void SessionStorageContextMojo::OnUserServiceConnectionError() {
  CHECK(false);
}

void SessionStorageContextMojo::BindSessionStorage(const std::string& namespace_id,
                                                   const url::Origin& origin,
                                                   mojom::LevelDBWrapperRequest request) {
  AreaKey area_key(namespace_id, origin);
  auto found = areas_.find(area_key);
  if (found != areas_.end()) {
    found->second->CallWhenReady(base::BindOnce(&BindRequest, std::move(request)));
    return;
  }

  auto area = base::MakeUnique<Area>(this, namespace_id, origin);
  area->CallWhenReady(base::BindOnce(&BindRequest, std::move(request)));
  areas_[area_key] = std::move(area);
}

}  // namespace content
