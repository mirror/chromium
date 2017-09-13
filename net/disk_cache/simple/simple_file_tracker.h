// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_DISK_CACHE_SIMPLE_SIMPLE_FILE_TRACKER_H_
#define NET_DISK_CACHE_SIMPLE_SIMPLE_FILE_TRACKER_H_

#include <stdint.h>
#include <algorithm>
#include <memory>
#include <unordered_map>

#include "base/files/file.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"
#include "net/base/net_export.h"
#include "net/disk_cache/simple/simple_entry_format.h"

namespace disk_cache {

class SimpleSynchronousEntry;

// This keeps track of all the files SimpleCache has open, across all the
// backends. It will eventually close some to prevent us from running out of
// file descriptors, but that's not yet implemented, to keep CL sizes down.
class NET_EXPORT_PRIVATE
    SimpleFileTracker /*: public base::RefCountedThreadSafe<SimpleFileTracker>*/
{
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
  void Register(const SimpleSynchronousEntry* owner,
                const base::FilePath& base_path,
                const EntryFileKey& key,
                SubFile subfile,
                base::File&& file);

  // Lends out a file to SimpleSynchronousEntry for use. SimpleFileTracker
  // will ensure that it doesn't close the FD until Release() is called
  // with the same parameters.
  base::File* Acquire(const SimpleSynchronousEntry* owner,
                      const EntryFileKey& key,
                      SubFile subfile);
  void Release(const SimpleSynchronousEntry* owner,
               const EntryFileKey& key,
               SubFile subfile);

  // Tells SimpleFileTracker that SimpleSynchronousEntry will not be interested
  // in the file further at all, so it can be closed and forgotten about.
  // It's OK to call this before Release (in which case the effect place takes
  // after the Release call).
  void Close(const SimpleSynchronousEntry* owner,
             const EntryFileKey& key,
             SubFile file);

  // Updates key->doom_generation to one not in use for the hash; it's the
  // caller's responsibility to update file names accordingly. The assumption is
  // also that the external mechanism (active_entries + entries_pending_doom_)
  // protects this from racing, in that the caller should rename the files
  // before reporting the doom as successful.
  //
  // Note that this is not implemented ATM, since it can't be implemented with
  // just knowledge of a single SimpleSynchronousEntry.
  void Doom(EntryFileKey* key);

 private:
  struct TrackedFiles {
    enum State {
      TF_CLOSED = 0,
      TF_REGISTERED = 1,
      TF_ACQUIRED = 2,
      TF_ACQUIRED_PENDING_CLOSE = 3,
    };

    TrackedFiles() {
      std::fill(state, state + kSimpleEntrySubFileCount, TF_CLOSED);
    }

    bool AllClosed() const {
      for (int i = 0; i < kSimpleEntrySubFileCount; ++i)
        if (state[i] != TF_CLOSED)
          return false;
      return true;
    }

    const SimpleSynchronousEntry* owner;
    base::FilePath base_path;
    EntryFileKey key;
    // Some of these may be !IsValid(), if they are not open.
    base::File files[kSimpleEntrySubFileCount];
    State state[kSimpleEntrySubFileCount];
  };

  // This one assumes the entry exists.
  std::vector<TrackedFiles>::iterator Find(const SimpleSynchronousEntry* owner,
                                           const EntryFileKey& key);
  // TrackedFiles* FindOrInsertFor()

  // Handles state transition of closing file (when we are not deferring it),
  // and moves the file into *file_out.
  void PrepareClose(std::vector<TrackedFiles>::iterator owners_files,
                    int file_index,
                    base::File* file_out);

  base::Lock lock_;

  // We use pointers to SimpleSynchronousEntry as opaque keys. This is handy
  // as it avoids having to compare paths in case multiple backends use the
  // same key.
  //
  // (actually accessing them will not in general be thread safe, since FD
  //  stealing can take place in a different thread).
  std::unordered_map<uint64_t, std::vector<TrackedFiles>> tracked_files_;

  DISALLOW_COPY_AND_ASSIGN(SimpleFileTracker);
};

}  // namespace disk_cache

#endif  // NET_DISK_CACHE_SIMPLE_SIMPLE_FILE_TRACKER_H_
