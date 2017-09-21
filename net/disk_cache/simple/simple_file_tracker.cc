// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/disk_cache/simple/simple_file_tracker.h"

#include <memory>

namespace disk_cache {

SimpleFileTracker::SimpleFileTracker() {}

SimpleFileTracker::~SimpleFileTracker() {}

void SimpleFileTracker::Register(const SimpleSynchronousEntry* owner,
                                 const EntryFileKey& key,
                                 SubFile subfile,
                                 base::File file) {
  base::AutoLock hold_lock(lock_);

  // Make sure the list exists.
  // ### check # of allocs for default ctor here.
  auto insert_status = tracked_files_.insert(
      std::make_pair(key.entry_hash, std::vector<TrackedFiles>()));

  std::vector<TrackedFiles>& candidates = insert_status.first->second;

  // See if entry already exists, if not append.
  TrackedFiles* owners_files = nullptr;
  for (TrackedFiles& candidate : candidates) {
    if (candidate.owner == owner) {
      owners_files = &candidate;
      break;
    }
  }

  if (!owners_files) {
    candidates.emplace_back();
    owners_files = &candidates.back();
    owners_files->owner = owner;
    owners_files->key = key;
  }

  int file_index = static_cast<int>(subfile);
  DCHECK_EQ(TrackedFiles::TF_NO_REGISTRATION, owners_files->state[file_index]);
  owners_files->files[file_index] =
      std::make_unique<base::File>(std::move(file));
  owners_files->state[file_index] = TrackedFiles::TF_REGISTERED;
}

base::File* SimpleFileTracker::Acquire(const SimpleSynchronousEntry* owner,
                                       const EntryFileKey& key,
                                       SubFile subfile) {
  base::AutoLock hold_lock(lock_);
  std::vector<TrackedFiles>::iterator owners_files = Find(owner, key);
  int file_index = static_cast<int>(subfile);

  DCHECK_EQ(TrackedFiles::TF_REGISTERED, owners_files->state[file_index]);
  owners_files->state[file_index] = TrackedFiles::TF_ACQUIRED;
  return owners_files->files[file_index].get();
}

void SimpleFileTracker::Release(const SimpleSynchronousEntry* owner,
                                const EntryFileKey& key,
                                SubFile subfile) {
  std::unique_ptr<base::File> file_to_close;

  {
    base::AutoLock hold_lock(lock_);
    std::vector<TrackedFiles>::iterator owners_files = Find(owner, key);
    int file_index = static_cast<int>(subfile);

    DCHECK(owners_files->state[file_index] == TrackedFiles::TF_ACQUIRED ||
           owners_files->state[file_index] ==
               TrackedFiles::TF_ACQUIRED_PENDING_CLOSE);

    // Prepare to executed deferred close, if any.
    if (owners_files->state[file_index] ==
        TrackedFiles::TF_ACQUIRED_PENDING_CLOSE) {
      file_to_close = PrepareClose(owners_files, file_index);
    } else {
      owners_files->state[file_index] = TrackedFiles::TF_REGISTERED;
    }
  }

  // The destructor of file_to_close will close it if needed.
}

void SimpleFileTracker::Close(const SimpleSynchronousEntry* owner,
                              const EntryFileKey& key,
                              SubFile subfile) {
  std::unique_ptr<base::File> file_to_close;

  {
    base::AutoLock hold_lock(lock_);
    std::vector<TrackedFiles>::iterator owners_files = Find(owner, key);
    int file_index = static_cast<int>(subfile);

    DCHECK(owners_files->state[file_index] == TrackedFiles::TF_ACQUIRED ||
           owners_files->state[file_index] == TrackedFiles::TF_REGISTERED);

    if (owners_files->state[file_index] == TrackedFiles::TF_ACQUIRED) {
      // The FD is currently acquired, so we can't clean up the TrackedFiles,
      // just yet; even if this is the last close, so delay the close until it
      // gets released.
      owners_files->state[file_index] = TrackedFiles::TF_ACQUIRED_PENDING_CLOSE;
    } else {
      file_to_close = PrepareClose(owners_files, file_index);
    }
  }

  // The destructor of file_to_close will close it if needed. Thing to watch
  // for impl with stealing: race between bookkeeping above and actual
  // close --- the FD is still alive for it.
}

std::vector<SimpleFileTracker::TrackedFiles>::iterator SimpleFileTracker::Find(
    const SimpleSynchronousEntry* owner,
    const EntryFileKey& key) {
  auto candidates = tracked_files_.find(key.entry_hash);
  for (std::vector<TrackedFiles>::iterator i = candidates->second.begin();
       i != candidates->second.end(); ++i) {
    if (i->owner == owner)
      return i;
  }
  CHECK(false);
  return candidates->second.end();
}

std::unique_ptr<base::File> SimpleFileTracker::PrepareClose(
    std::vector<TrackedFiles>::iterator owners_files,
    int file_index) {
  std::unique_ptr<base::File> file_out =
      std::move(owners_files->files[file_index]);
  owners_files->state[file_index] = TrackedFiles::TF_NO_REGISTRATION;
  if (owners_files->NoActiveRegistrations())
    tracked_files_.find(owners_files->key.entry_hash)
        ->second.erase(owners_files);
  return file_out;
}

}  // namespace disk_cache
