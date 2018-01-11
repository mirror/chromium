// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOM_STORAGE_LEVELDB_REF_COUNTED_MAP_H_
#define CONTENT_BROWSER_DOM_STORAGE_LEVELDB_REF_COUNTED_MAP_H_

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/optional.h"
#include "content/browser/leveldb_wrapper_impl.h"
#include "content/common/leveldb_wrapper.mojom.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"

namespace content {

// Handles deleting the map when the refcount == 0, and moving observers
// between deep clones.
class LevelDBRefCountedMap {
 public:
  LevelDBRefCountedMap(leveldb::mojom::LevelDBDatabase* database,
                       const std::string& prefix,
                       int32_t refcnt,
                       LevelDBWrapperImpl::Delegate* delegate,
                       const LevelDBWrapperImpl::Options& options);
  virtual ~LevelDBRefCountedMap();

  LevelDBWrapperImpl* impl() { return wrapper_impl_.get(); }

  // Not valid until |initialized()| returns true.
  int32_t map_refcount() { return map_refcount_; }

  void AddMapReference();
  // The map will automatically be deleted when the refcount == 0.
  void RemoveMapReference();

  // Automatically removes a reference from this map and sets the reference of
  // the new map to 1. Moves the given observer from this map to the new map.
//  std::unique_ptr<LevelDBRefCountedMap> CloneDeepCopy(
//      const std::string& new_prefix,
//      LevelDBWrapperImpl::Delegate* delegate,
//      const LevelDBWrapperImpl::Options& options,
//      base::Optional<mojo::PtrId> observer_to_move,
//      mojo::PtrId* new_observer_id);

 private:
  // Constructor used for deep copy clone.
  LevelDBRefCountedMap(std::unique_ptr<LevelDBWrapperImpl> wrapper);

  int32_t map_refcount_;

  std::unique_ptr<LevelDBWrapperImpl> wrapper_impl_;

  DISALLOW_COPY_AND_ASSIGN(LevelDBRefCountedMap);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOM_STORAGE_LEVELDB_REF_COUNTED_MAP_H_
