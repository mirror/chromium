// Copyright 2017 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef THIRD_PARTY_LEVELDATABASE_LEVELDB_CHROMIUM_H_
#define THIRD_PARTY_LEVELDATABASE_LEVELDB_CHROMIUM_H_

#include "leveldb/cache.h"

namespace leveldb {
namespace chrome {

// Return the shared leveldb block cache for web APIs. The caller *does not*
// own the returned instance.
extern Cache* GetSharedWebBlockCache();

// Return the shared leveldb block cache for browser (non web) APIs. The caller
// *does not* own the returned instance.
extern Cache* GetSharedBrowserBlockCache();

}  // namespace chrome
}  // namespace leveldb

#endif  // THIRD_PARTY_LEVELDATABASE_LEVELDB_CHROMIUM_H_
