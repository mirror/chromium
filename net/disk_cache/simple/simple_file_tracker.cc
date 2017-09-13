// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/disk_cache/simple/simple_file_tracker.h"

namespace disk_cache {

SimpleFileTracker::SimpleFileTracker() {}

SimpleFileTracker::~SimpleFileTracker() {}

void SimpleFileTracker::Register(SimpleSynchronousEntry* owner,
                                 const base::FilePath& base_path,
                                 const EntryFileKey& key,
                                 SubFile subfile,
                                 base::File&& file) {
  base::AutoLock hold_lock(lock_);
  auto insert_status = tracked_files_.insert(
      std::make_pair(owner, std::unique_ptr<TrackedFiles>(nullptr)));
  if (insert_status.second) {
    // Actually inserted.
    std::unique_ptr<TrackedFiles> new_owners_files =
        std::make_unique<TrackedFiles>();
    new_owners_files->base_path = base_path;
    new_owners_files->key = key;
    insert_status.first->second = std::move(new_owners_files);
  }
  TrackedFiles* owners_files = insert_status.first->second.get();
  int file_index = static_cast<int>(subfile);
  DCHECK_EQ(TrackedFiles::TF_CLOSED, owners_files->state[file_index]);
  owners_files->files[file_index] = std::move(file);
  owners_files->state[file_index] = TrackedFiles::TF_REGISTERED;
}

base::File* SimpleFileTracker::Acquire(SimpleSynchronousEntry* owner,
                                       SubFile subfile) {
  base::AutoLock hold_lock(lock_);
  auto iter = tracked_files_.find(owner);
  if (iter == tracked_files_.end() || !iter->second) {
    LOG(DFATAL) << "Entry missing in SimpleFileTracker::Acquire??";
    return nullptr;
  }
  TrackedFiles* owners_files = iter->second.get();
  int file_index = static_cast<int>(subfile);

  DCHECK_EQ(TrackedFiles::TF_REGISTERED, owners_files->state[file_index]);
  owners_files->state[file_index] = TrackedFiles::TF_ACQUIRED;
  return &owners_files->files[file_index];
}

void SimpleFileTracker::Release(SimpleSynchronousEntry* owner,
                                SubFile subfile) {
  base::File file_to_close;

  {
    base::AutoLock hold_lock(lock_);
    auto iter = tracked_files_.find(owner);
    if (iter == tracked_files_.end() || !iter->second) {
      LOG(DFATAL) << "Entry missing in SimpleFileTracker::Release??";
      return;
    }
    TrackedFiles* owners_files = iter->second.get();
    int file_index = static_cast<int>(subfile);

    DCHECK(owners_files->state[file_index] == TrackedFiles::TF_ACQUIRED ||
           owners_files->state[file_index] ==
               TrackedFiles::TF_ACQUIRED_PENDING_CLOSE);

    // Prepare to executed deferred close, if any.
    if (owners_files->state[file_index] ==
        TrackedFiles::TF_ACQUIRED_PENDING_CLOSE) {
      file_to_close = std::move(owners_files->files[file_index]);
      owners_files->state[file_index] = TrackedFiles::TF_CLOSED;
      if (owners_files->AllClosed())
        tracked_files_.erase(iter);
    } else {
      owners_files->state[file_index] = TrackedFiles::TF_REGISTERED;
    }
  }

  // The destructor of file_to_close will close it if needed.
}

void SimpleFileTracker::Close(SimpleSynchronousEntry* owner, SubFile subfile) {
  base::File file_to_close;

  {
    base::AutoLock hold_lock(lock_);
    auto iter = tracked_files_.find(owner);
    if (iter == tracked_files_.end() || !iter->second) {
      LOG(DFATAL) << "Entry missing in SimpleFileTracker::Close??";
      return;
    }
    TrackedFiles* owners_files = iter->second.get();
    int file_index = static_cast<int>(subfile);

    DCHECK(owners_files->state[file_index] == TrackedFiles::TF_ACQUIRED ||
           owners_files->state[file_index] == TrackedFiles::TF_REGISTERED);

    if (owners_files->state[file_index] == TrackedFiles::TF_ACQUIRED) {
      // The FD is currently acquired, so we can't clean up the TrackedFiles,
      // just yet; even if this is the last close, so delay the close until it
      // gets released.
      owners_files->state[file_index] = TrackedFiles::TF_ACQUIRED_PENDING_CLOSE;
    } else {
      file_to_close = std::move(owners_files->files[file_index]);
      owners_files->state[file_index] = TrackedFiles::TF_CLOSED;
      if (owners_files->AllClosed())
        tracked_files_.erase(iter);
    }
  }

  // The destructor of file_to_close will close it if needed. Thing to watch
  // for impl with stealing: race between bookkeeping above and actual
  // close --- the FD is still alive for it.
}

}  // namespace disk_cache
