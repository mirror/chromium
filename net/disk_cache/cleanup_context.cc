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
#include "base/lazy_instance.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "base/synchronization/lock.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/thread_checker.h"
#include "net/disk_cache/disk_cache.h"

namespace disk_cache {

namespace {

typedef std::unordered_map<base::FilePath, CleanupContext*> ContextMap;
struct AllContexts {
  ContextMap map;

  // Since clients can potentially call CreateCacheBackend from multiple
  // threads, we need to lock the map keeping track of cleanup contexts
  // for these backends. Our overall strategy is to have TryMakeContext
  // acts as an arbitrator --- whatever thread grabs one, gets to operate
  // on the context freely until it gets destroyed.
  base::Lock lock;
};

static base::LazyInstance<AllContexts>::Leaky g_all_contexts;

}  // namespace.

CleanupContext* CleanupContext::TryMakeContext(
    const base::FilePath& path,
    base::OnceClosure retry_closure) {
  AllContexts* all_contexts = g_all_contexts.Pointer();
  base::AutoLock lock(all_contexts->lock);

  std::pair<ContextMap::iterator, bool> insert_result =
      all_contexts->map.insert(
          std::pair<base::FilePath, CleanupContext*>(path, nullptr));
  if (insert_result.second) {
    insert_result.first->second = new CleanupContext(path);
    return insert_result.first->second;
  } else {
    insert_result.first->second->AddPostCleanupCallbackImpl(
        std::move(retry_closure));
    return nullptr;
  }
}

void CleanupContext::AddPostCleanupCallback(PostCleanupCallback cb) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(seq_checker_);
  // Despite the sequencing requirement we need to grab the table lock since
  // this may otherwise race against TryMakeContext.
  base::AutoLock lock(g_all_contexts.Get().lock);
  AddPostCleanupCallbackImpl(std::move(cb));
}

void CleanupContext::AddPostCleanupCallbackImpl(PostCleanupCallback cb) {
  post_cleanup_cbs_.push_back(
      std::make_pair(base::SequencedTaskRunnerHandle::Get(), std::move(cb)));
}

CleanupContext::CleanupContext(const base::FilePath& path) : path_(path) {}

CleanupContext::~CleanupContext() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(seq_checker_);

  {
    AllContexts* all_contexts = g_all_contexts.Pointer();
    base::AutoLock lock(all_contexts->lock);
    int rv = all_contexts->map.erase(path_);
    DCHECK_EQ(1, rv);
  }

  while (!post_cleanup_cbs_.empty()) {
    post_cleanup_cbs_.back().first->PostTask(
        FROM_HERE, std::move(post_cleanup_cbs_.back().second));
    post_cleanup_cbs_.pop_back();
  }
}

}  // namespace disk_cache
