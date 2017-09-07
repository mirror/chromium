// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/disk_cache/simple/simple_file_tracker.h"

namespace disk_cache {

SimpleFileTracker::SimpleFileTracker() {}

SimpleFileTracker::~SimpleFileTracker() {}

void SimpleFileTracker::Register(const EntryFileKey& key,
                                 SubFile subfile,
                                 std::unique_ptr<base::File> file) {
  files_[static_cast<int>(subfile)] = std::move(file);
}

base::File* SimpleFileTracker::Acquire(const EntryFileKey& key,
                                       SubFile subfile) {
  return files_[static_cast<int>(subfile)].get();
}

void SimpleFileTracker::Release(const EntryFileKey& key, SubFile subfile) {
  // Don't need to do anything in the fake impl. Real one would need to:
  // 1) If Close has been called on it before, close the file and remove all
  //    references to it from our data structures.
  // 2) Otherwise move it to list of things we can close temporarily on demand.
}

void SimpleFileTracker::Close(const EntryFileKey& key, SubFile subfile) {
  files_[static_cast<int>(subfile)]->Close();
}

}  // namespace disk_cache
