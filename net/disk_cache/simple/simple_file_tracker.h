// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_DISK_CACHE_SIMPLE_SIMPLE_FILE_TRACKER_H_
#define NET_DISK_CACHE_SIMPLE_SIMPLE_FILE_TRACKER_H_

#include <stdint.h>

#include "base/macros.h"
#include "net/base/net_export.h"

namespace disk_cache {

// This will eventually help keep track of what files the backend has open, so
// we don't use an unreasonable number of FDs. Right now it just holds some
// types we've started using in preparation.
class NET_EXPORT_PRIVATE SimpleFileTracker {
 public:
  enum class SubFile { FILE_0, FILE_1, FILE_SPARSE };

  struct EntryFileKey {
    EntryFileKey() : entry_hash(0), doom_generation(0) {}
    explicit EntryFileKey(uint64_t hash)
        : entry_hash(hash), doom_generation(0) {}

    uint64_t entry_hash;

    // In case of a hash collision, there may be multiple SimpleEntryImpl's
    // around which have the same entry_hash but different key. In that case,
    // we doom all but the most recent one and this number will eventually be
    // used to name the files for the doomed ones.
    // 0 here means the entry is the active one, and is the only value that's
    // presently in use here.
    uint32_t doom_generation;
  };

  DISALLOW_COPY_AND_ASSIGN(SimpleFileTracker);
};

}  // namespace disk_cache

#endif  // NET_DISK_CACHE_SIMPLE_SIMPLE_FILE_TRACKER_H_
