// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Internal helper used to sequence cleanup and reuse of cache directories.
// One of these is created before each backend, and is kept alive till both
// the backend is destroyed and all of its work is done by its refcount,
// which keeps track of outstanding work. That refcount is expected to only be
// updated from the I/O thread or its equivalent.

#ifndef NET_DISK_CACHE_CLEANUP_CONTEXT_H_
#define NET_DISK_CACHE_CLEANUP_CONTEXT_H_

#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/sequence_checker.h"
#include "net/disk_cache/disk_cache.h"

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace disk_cache {

class CleanupContext : public base::RefCounted<CleanupContext> {
 public:
  // Either returns a fresh context for |path| if none exist, or
  // will eventually post |retry_closure| to the calling thread,
  // and return null.
  //
  // If new, will have reference count of zero.
  static CleanupContext* TryMakeContext(const base::FilePath& path,
                                        base::OnceClosure retry_closure);

  // Register a callback to be posted after all the work of associated
  // context is complete (which will result in destruction of this context).
  // Should only be called by owner, on its I/O-thread-like execution context,
  // and will in turn eventually post |cb| there.
  void AddPostCleanupCallback(PostCleanupCallback cb);

 private:
  friend class base::RefCounted<CleanupContext>;
  CleanupContext(const base::FilePath& path);
  ~CleanupContext();

  void AddPostCleanupCallbackImpl(PostCleanupCallback cb);

  base::FilePath path_;

  // Since it's possible that a different thread may want to create a
  // cache for a reused path, we keep track of runners contexts of
  // post-cleanup callbacks.
  std::vector<
      std::pair<scoped_refptr<base::SequencedTaskRunner>, PostCleanupCallback>>
      post_cleanup_cbs_;

  // We expect only TryMakeContext to be multithreaded, everything
  // else should be sequenced.
  SEQUENCE_CHECKER(seq_checker_);

  DISALLOW_COPY_AND_ASSIGN(CleanupContext);
};

}  // namespace disk_cache

#endif  // NET_DISK_CACHE_CLEANUP_CONTEXT_H_
