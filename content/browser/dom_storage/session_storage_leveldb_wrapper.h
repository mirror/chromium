// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOM_STORAGE_SESSION_STORAGE_LEVELDB_WRAPPER_H_
#define CONTENT_BROWSER_DOM_STORAGE_SESSION_STORAGE_LEVELDB_WRAPPER_H_

#include <vector>

#include "base/callback.h"
#include "base/optional.h"
#include "base/memory/ref_counted.h"
#include "content/browser/leveldb_wrapper_impl.h"
#include "content/common/leveldb_wrapper.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace content {
class SessionStorageContextMojo;

// This class is made to create a correct ordering of the calls to the leveldb
// wrapper and forking.
// This class does the following:
// * Queues all calls until OnDatabaseLoadComplete is called.
// * Writes session storage metadata like db version.
class CONTENT_EXPORT SessionStorageLevelDBWrapper
    : public mojom::LevelDBWrapper {
 public:
  class LevelDBRefCountedMap;

  SessionStorageLevelDBWrapper(SessionStorageContextMojo* context);
  SessionStorageLevelDBWrapper(SessionStorageContextMojo* context,
                               scoped_refptr<LevelDBRefCountedMap> map);
  ~SessionStorageLevelDBWrapper() override;

  void Bind(mojom::LevelDBWrapperRequest request);

  bool IsBound() const { return binding_.is_bound(); }

  bool IsSharedMapConnected() const { return shared_map_->IsConnected(); }
  void OnDBConnectionComplete(leveldb::mojom::LevelDBDatabase* database,
                              std::vector<uint8_t> map_prefix);

  base::OnceClosure CreateStartClosure();
  void StartOperations();

  LevelDBRefCountedMap* ref_counted_map() { return shared_map_.get(); }

  std::unique_ptr<SessionStorageLevelDBWrapper> Clone();

  // LevelDBWrapper:
  void AddObserver(mojom::LevelDBObserverAssociatedPtrInfo observer) override;
  void Put(const std::vector<uint8_t>& key,
           const std::vector<uint8_t>& value,
           const base::Optional<std::vector<uint8_t>>& client_old_value,
           const std::string& source,
           PutCallback callback) override;
  void Delete(const std::vector<uint8_t>& key,
              const base::Optional<std::vector<uint8_t>>& client_old_value,
              const std::string& source,
              DeleteCallback callback) override;
  void DeleteAll(const std::string& source,
                 DeleteAllCallback callback) override;
  void Get(const std::vector<uint8_t>& key, GetCallback callback) override;
  void GetAll(
      mojom::LevelDBWrapperGetAllCallbackAssociatedPtrInfo complete_callback,
      GetAllCallback callback) override;

  class CONTENT_EXPORT LevelDBRefCountedMap final
      : public LevelDBWrapperImpl::Delegate,
        public base::RefCounted<LevelDBRefCountedMap> {
   public:

    bool IsConnected() { return level_db_wrapper_ptr_ != nullptr; }
    void Connect(leveldb::mojom::LevelDBDatabase* database,
                 std::vector<uint8_t> map_prefix);

    int binding_count() { return binding_count_; }

    void AddBindingReference() { ++binding_count_; }
    void RemoveBindingReference();

    LevelDBWrapperImpl* level_db_wrapper() { return level_db_wrapper_ptr_; }

    // Note: this is irrelevant, as the parent wrapper is handling binding.
    void OnNoBindings() override {}

    std::vector<leveldb::mojom::BatchedOperationPtr> PrepareToCommit() override;

    void DidCommit(leveldb::mojom::DatabaseError error) override;

   private:
    template <typename T, typename... Args>
    friend scoped_refptr<T> base::MakeRefCounted(Args&&... args);
    friend class base::RefCounted<LevelDBRefCountedMap>;
    friend class SessionStorageLevelDBWrapper;

    LevelDBRefCountedMap(SessionStorageContextMojo* context);
    LevelDBRefCountedMap(std::vector<uint8_t> map_prefix,
                         LevelDBRefCountedMap* forking_from);
    ~LevelDBRefCountedMap() override;

    LevelDBWrapperImpl::Options GetOptions();

    SessionStorageContextMojo* context_;
    int binding_count_ = 0;
    std::unique_ptr<LevelDBWrapperImpl> wrapper_impl_;
    // Holds the same value as |wrapper_impl_|. The reason for this is that
    // during destruction of the LevelDBWrapperImpl instance we might still get
    // called and need access  to the LevelDBWrapperImpl instance. The
    // unique_ptr could already be null, but this field should still be valid.
    // TODO(dmurph): Change delegate ownership so this doesn't have to be done.
    LevelDBWrapperImpl* level_db_wrapper_ptr_ = nullptr;

    DISALLOW_COPY_AND_ASSIGN(LevelDBRefCountedMap);
  };

 private:
  void OnConnectionError();

  void ForkToNewMap();

  // Note: The case where the refcount of this map is 1 is used for shallow
  // cloning logic. This is the only scoped_refptr that should be holding this
  // object.
  scoped_refptr<LevelDBRefCountedMap> shared_map_;
  SessionStorageContextMojo* context_;

  bool is_connected_ = false;
  std::vector<base::OnceClosure> on_load_complete_tasks_;

  std::vector<mojo::PtrId> observer_ptrs_;
  mojo::Binding<mojom::LevelDBWrapper> binding_;

  base::WeakPtrFactory<SessionStorageLevelDBWrapper> ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(SessionStorageLevelDBWrapper);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOM_STORAGE_SESSION_STORAGE_LEVELDB_WRAPPER_H_
