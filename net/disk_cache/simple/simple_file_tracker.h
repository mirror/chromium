// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_DISK_CACHE_SIMPLE_SIMPLE_FILE_TRACKER_H_
#define NET_DISK_CACHE_SIMPLE_SIMPLE_FILE_TRACKER_H_

#include <stdint.h>
#include <memory>

#include "base/files/file.h"
#include "base/macros.h"
#include "net/base/net_export.h"

namespace disk_cache {

// This will eventually help keep track of what files the backend has open, so
// we don't use an unreasonable number of FDs. Right now it's a fake
// implementation that owns the files for a single entry, to help transition
// code incrementally.
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

  SimpleFileTracker();
  ~SimpleFileTracker();

  // Established |file| as what's backing |subfile| for |key|. This is intended
  // to be called when SimpleSynchronousEntry first sets up the file to transfer
  // its ownership to SimpleFileTracker.
  void Register(const EntryFileKey& key,
                SubFile subfile,
                std::unique_ptr<base::File> file);

  // Lends out a file to SimpleSynchronousEntry for use. SimpleFileTracker
  // will ensure that it doesn't close the FD until Release() is called
  // with the same parameters.
  base::File* Acquire(const EntryFileKey& key, SubFile subfile);
  void Release(const EntryFileKey& key, SubFile subfile);

  // Tells SimpleFileTracker that SimpleSynchronousEntry will not be interested
  // in the file further at all, so it can be closed and forgotten about.
  // It's OK to call this before Release (in which case the effect place takes
  // after the Release call).
  void Close(const EntryFileKey& key, SubFile file);

  // Updates key->doom_generation to one not in use for the hash; it's the
  // caller's responsibility to update file names accordingly. The assumption is
  // also that the external mechanism (entries_pending_doom_) protects this from
  // racing, in that the caller should rename the files before reporting the
  // doom as successful.
  //
  // Note that this is not implemented ATM, since it can't be implemented with
  // just knowledge of a single SimpleSynchronousEntry.
  void Doom(EntryFileKey* key);

 private:
  std::unique_ptr<base::File> files_[3];

  DISALLOW_COPY_AND_ASSIGN(SimpleFileTracker);
};

}  // namespace disk_cache

#endif  // NET_DISK_CACHE_SIMPLE_SIMPLE_FILE_TRACKER_H_
