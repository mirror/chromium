// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Internal helper used to sequence cleanup and reuse of cache directories
// among different objects. Its kept alive by an I/O-thread-only refcount
// of outstanding work + backend existence.

#ifndef NET_DISK_CACHE_CLEANUP_CONTEXT_H_
#define NET_DISK_CACHE_CLEANUP_CONTEXT_H_

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"
#include "net/disk_cache/disk_cache.h"

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace disk_cache {

class CleanupContext : public base::RefCounted<CleanupContext> {
 public:
  // If new, will have reference count of zero.
  static CleanupContext* MakeOrGetContextForPath(const base::FilePath& path);

  static CleanupContext* GetExistingForPath(const base::FilePath& path);

  void SetPostCleanupCallback(PostCleanupCallback cb);
  void SetTaskRunnerForDisk(
      const scoped_refptr<base::SequencedTaskRunner>& task_runner);

 private:
  friend class base::RefCounted<CleanupContext>;
  CleanupContext(const base::FilePath& path);
  ~CleanupContext();

  base::FilePath path_;
  PostCleanupCallback post_cleanup_cb_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_for_cb_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_for_disk_;

  // Everything must be called from the I/O thread.
  base::ThreadChecker io_thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(CleanupContext);
};

}  // namespace disk_cache

#endif  // NET_DISK_CACHE_CLEANUP_CONTEXT_H_
