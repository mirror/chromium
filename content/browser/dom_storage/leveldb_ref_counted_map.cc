// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/dom_storage/leveldb_ref_counted_map.h"

#include "base/bind.h"

namespace content {
namespace {

const char kLevelDBRefCountedMapSource[] = "LevelDBRefCountedMap";
void NoOpSuccess(bool success) {}

}  // namespace

LevelDBRefCountedMap::LevelDBRefCountedMap(
    leveldb::mojom::LevelDBDatabase* database,
    const std::string& prefix,
    int32_t refcnt,
    LevelDBWrapperImpl::Delegate* delegate,
    const LevelDBWrapperImpl::Options& options)
    : map_refcount_(refcnt),
      wrapper_impl_(std::make_unique<LevelDBWrapperImpl>(database,
                                                         prefix,
                                                         delegate,
                                                         options)) {}

LevelDBRefCountedMap::~LevelDBRefCountedMap() = default;

void LevelDBRefCountedMap::AddMapReference() {
  ++map_refcount_;
}

void LevelDBRefCountedMap::RemoveMapReference() {
  DCHECK_GT(map_refcount_, 0);
  --map_refcount_;
  if (map_refcount_ <= 0) {
    wrapper_impl_->DeleteAll(kLevelDBRefCountedMapSource,
                             base::BindOnce(&NoOpSuccess));
  }
}

//std::unique_ptr<LevelDBRefCountedMap> LevelDBRefCountedMap::CloneDeepCopy(
//    const std::string& new_prefix,
//    LevelDBWrapperImpl::Delegate* delegate,
//    const LevelDBWrapperImpl::Options& options,
//    base::Optional<mojo::PtrId> observer_to_move,
//    mojo::PtrId* new_observer_id) {
//  mojom::LevelDBObserverAssociatedPtr ptr_to_move;
//  if (observer_to_move) {
//    DCHECK(wrapper_impl_->HasObserver(observer_to_move.value()));
//    ptr_to_move = wrapper_impl_->RemoveObserver(observer_to_move.value());
//  }
//  std::unique_ptr<LevelDBWrapperImpl> fork =
//      wrapper_impl_->ForkToNewPrefix(new_prefix, delegate, options);
//  RemoveMapReference();
//
//  if (observer_to_move) {
//    *new_observer_id = fork->AddObserver(std::move(ptr_to_move));
//  }
//  return std::make_unique<LevelDBRefCountedMap>(std::move(fork));
//}

LevelDBRefCountedMap::LevelDBRefCountedMap(
    std::unique_ptr<LevelDBWrapperImpl> wrapper)
    : map_refcount_(1), wrapper_impl_(std::move(wrapper)) {}

}  // namespace content
