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
  // Don't need to do anything in the fake impl. Real one would need
  // to decrement the acquisition count, and if it's zero, make file potentially
  // eligible for closure.
}

void SimpleFileTracker::Close(const EntryFileKey& key, SubFile subfile) {
  files_[static_cast<int>(subfile)]->Close();
}

}  // namespace disk_cache
