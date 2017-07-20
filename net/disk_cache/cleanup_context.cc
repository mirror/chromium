// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Internal helper used to sequence cleanup and reuse of cache directories
// among different objects.

#include "net/disk_cache/cleanup_context.h"

#include <unordered_map>
#include <utility>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/thread_checker.h"
#include "net/disk_cache/disk_cache.h"

namespace disk_cache {

namespace {

typedef std::unordered_map<base::FilePath, CleanupContext*> ContextMap;

ContextMap* g_context_map;
ContextMap* context_map() {
  if (!g_context_map)
    g_context_map = new ContextMap;
  return g_context_map;
}

}  // namespace.

CleanupContext* CleanupContext::MakeOrGetContextForPath(
    const base::FilePath& path) {
  std::pair<ContextMap::iterator, bool> insert_result = context_map()->insert(
      std::pair<base::FilePath, CleanupContext*>(path, nullptr));
  if (insert_result.second)
    insert_result.first->second = new CleanupContext(path);
  return insert_result.first->second;
}

CleanupContext* CleanupContext::GetExistingForPath(const base::FilePath& path) {
  ContextMap* map = context_map();
  auto iter = map->find(path);
  if (iter != map->end())
    return iter->second;
  return nullptr;
}

void CleanupContext::SetPostCleanupCallback(PostCleanupCallback cb) {
  DCHECK(io_thread_checker_.CalledOnValidThread());
  if (!post_cleanup_cb_.is_null()) {
    LOG(DFATAL) << "Attempt to set two post-clean up callbacks on same backend";
  }
  task_runner_for_cb_ = base::SequencedTaskRunnerHandle::Get();
  post_cleanup_cb_ = std::move(cb);
}

void CleanupContext::SetTaskRunnerForDisk(
    const scoped_refptr<base::SequencedTaskRunner>& task_runner) {
  task_runner_for_disk_ = task_runner;
}

CleanupContext::CleanupContext(const base::FilePath& path) : path_(path) {
  DCHECK(io_thread_checker_.CalledOnValidThread());
}

CleanupContext::~CleanupContext() {
  DCHECK(io_thread_checker_.CalledOnValidThread());
  int rv = context_map()->erase(path_);
  DCHECK_EQ(1, rv);

  if (!post_cleanup_cb_.is_null())
    task_runner_for_cb_->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(post_cleanup_cb_), task_runner_for_disk_));
}

}  // namespace disk_cache
