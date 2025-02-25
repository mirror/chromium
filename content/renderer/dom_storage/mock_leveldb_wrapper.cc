// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/dom_storage/mock_leveldb_wrapper.h"

namespace content {

MockLevelDBWrapper::MockLevelDBWrapper() {}
MockLevelDBWrapper::~MockLevelDBWrapper() {}

void MockLevelDBWrapper::OpenLocalStorage(
    const url::Origin& origin,
    mojom::LevelDBWrapperRequest database) {
  bindings_.AddBinding(this, std::move(database));
}

void MockLevelDBWrapper::OpenSessionStorage(
    const std::string& namespace_id,
    const url::Origin& origin,
    mojom::LevelDBWrapperRequest database) {
  bindings_.AddBinding(this, std::move(database));
}

void MockLevelDBWrapper::AddObserver(
    mojom::LevelDBObserverAssociatedPtrInfo observer) {}

void MockLevelDBWrapper::Put(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& value,
    const base::Optional<std::vector<uint8_t>>& client_old_value,
    const std::string& source,
    PutCallback callback) {
  observed_put_ = true;
  observed_key_ = key;
  observed_value_ = value;
  observed_source_ = source;
  pending_callbacks_.push_back(std::move(callback));
}

void MockLevelDBWrapper::Delete(
    const std::vector<uint8_t>& key,
    const base::Optional<std::vector<uint8_t>>& client_old_value,
    const std::string& source,
    DeleteCallback callback) {
  observed_delete_ = true;
  observed_key_ = key;
  observed_source_ = source;
  pending_callbacks_.push_back(std::move(callback));
}

void MockLevelDBWrapper::DeleteAll(const std::string& source,
                                   DeleteAllCallback callback) {
  observed_delete_all_ = true;
  observed_source_ = source;
  pending_callbacks_.push_back(std::move(callback));
}

void MockLevelDBWrapper::Get(const std::vector<uint8_t>& key,
                             GetCallback callback) {}

void MockLevelDBWrapper::GetAll(
    mojom::LevelDBWrapperGetAllCallbackAssociatedPtrInfo complete_callback,
    GetAllCallback callback) {
  mojom::LevelDBWrapperGetAllCallbackAssociatedPtr complete_ptr;
  complete_ptr.Bind(std::move(complete_callback));
  pending_callbacks_.push_back(base::BindOnce(
      &mojom::LevelDBWrapperGetAllCallback::Complete, std::move(complete_ptr)));

  observed_get_all_ = true;
  std::vector<mojom::KeyValuePtr> all;
  for (const auto& it : get_all_return_values_) {
    mojom::KeyValuePtr kv = mojom::KeyValue::New();
    kv->key = it.first;
    kv->value = it.second;
    all.push_back(std::move(kv));
  }
  std::move(callback).Run(leveldb::mojom::DatabaseError::OK, std::move(all));
}

}  // namespace content
