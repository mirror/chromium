// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/metrics/field_trial.h"
#include "base/single_thread_task_runner.h"
#include "base/task_scheduler/task_scheduler.h"
#include "net/base/cache_type.h"
#include "net/base/net_errors.h"
#include "net/disk_cache/blockfile/backend_impl.h"
#include "net/disk_cache/cache_util.h"
#include "net/disk_cache/cleanup_context.h"
#include "net/disk_cache/disk_cache.h"
#include "net/disk_cache/memory/mem_backend_impl.h"
#include "net/disk_cache/simple/simple_backend_impl.h"

namespace {

// Builds an instance of the backend depending on platform, type, experiments
// etc. Takes care of the retry state. This object will self-destroy when
// finished.
class CacheCreator {
 public:
  CacheCreator(const base::FilePath& path,
               bool force,
               int max_bytes,
               net::CacheType type,
               net::BackendType backend_type,
               uint32_t flags,
               const scoped_refptr<base::SingleThreadTaskRunner>& thread,
               net::NetLog* net_log,
               std::unique_ptr<disk_cache::Backend>* backend,
               disk_cache::PostCleanupCallback post_cleanup_callback,
               const net::CompletionCallback& callback);

  int TryMakeCleanupContextAndRun();

 private:
  // Creates the backend, the cleanup context for it having been already
  // established.
  int Run();

  ~CacheCreator();

  void DoCallback(int result);

  void OnIOComplete(int result);

  const base::FilePath path_;
  bool force_;
  bool retry_;
  int max_bytes_;
  net::CacheType type_;
  net::BackendType backend_type_;
#if !defined(OS_ANDROID)
  uint32_t flags_;
#endif
  scoped_refptr<base::SingleThreadTaskRunner> thread_;
  std::unique_ptr<disk_cache::Backend>* backend_;
  disk_cache::PostCleanupCallback post_cleanup_callback_;
  net::CompletionCallback callback_;
  std::unique_ptr<disk_cache::Backend> created_cache_;
  net::NetLog* net_log_;
  scoped_refptr<disk_cache::CleanupContext> cleanup_context_;

  DISALLOW_COPY_AND_ASSIGN(CacheCreator);
};

CacheCreator::CacheCreator(
    const base::FilePath& path,
    bool force,
    int max_bytes,
    net::CacheType type,
    net::BackendType backend_type,
    uint32_t flags,
    const scoped_refptr<base::SingleThreadTaskRunner>& thread,
    net::NetLog* net_log,
    std::unique_ptr<disk_cache::Backend>* backend,
    disk_cache::PostCleanupCallback post_cleanup_callback,
    const net::CompletionCallback& callback)
    : path_(path),
      force_(force),
      retry_(false),
      max_bytes_(max_bytes),
      type_(type),
      backend_type_(backend_type),
#if !defined(OS_ANDROID)
      flags_(flags),
#endif
      thread_(thread),
      backend_(backend),
      post_cleanup_callback_(std::move(post_cleanup_callback)),
      callback_(callback),
      net_log_(net_log) {
}

CacheCreator::~CacheCreator() {}

int CacheCreator::Run() {
#if defined(OS_ANDROID)
  static const bool kSimpleBackendIsDefault = true;
#else
  static const bool kSimpleBackendIsDefault = false;
#endif
  if (backend_type_ == net::CACHE_BACKEND_SIMPLE ||
      (backend_type_ == net::CACHE_BACKEND_DEFAULT &&
       kSimpleBackendIsDefault)) {
    disk_cache::SimpleBackendImpl* simple_cache =
        new disk_cache::SimpleBackendImpl(path_, cleanup_context_.get(),
                                          max_bytes_, type_, thread_, net_log_);
    created_cache_.reset(simple_cache);
    return simple_cache->Init(
        base::Bind(&CacheCreator::OnIOComplete, base::Unretained(this)));
  }

// Avoid references to blockfile functions on Android to reduce binary size.
#if defined(OS_ANDROID)
  return net::ERR_FAILED;
#else
  disk_cache::BackendImpl* new_cache = new disk_cache::BackendImpl(
      path_, cleanup_context_.get(), thread_, net_log_);
  created_cache_.reset(new_cache);
  new_cache->SetMaxSize(max_bytes_);
  new_cache->SetType(type_);
  new_cache->SetFlags(flags_);
  int rv = new_cache->Init(
      base::Bind(&CacheCreator::OnIOComplete, base::Unretained(this)));
  DCHECK_EQ(net::ERR_IO_PENDING, rv);
  return rv;
#endif
}

int CacheCreator::TryMakeCleanupContextAndRun() {
  // We want to hang on to this while we exist, so that in case we retry,
  // the user post_cleanup_callback gets called after the second try, not
  // first one.
  cleanup_context_ = disk_cache::CleanupContext::TryMakeContext(
      path_, base::BindOnce(
                 base::IgnoreResult(&CacheCreator::TryMakeCleanupContextAndRun),
                 base::Unretained(this)));
  if (!cleanup_context_)
    return net::ERR_IO_PENDING;
  if (!post_cleanup_callback_.is_null())
    cleanup_context_->AddPostCleanupCallback(std::move(post_cleanup_callback_));
  return Run();
}

void CacheCreator::DoCallback(int result) {
  DCHECK_NE(net::ERR_IO_PENDING, result);
  if (result == net::OK) {
    *backend_ = std::move(created_cache_);
  } else {
    LOG(ERROR) << "Unable to create cache";
    created_cache_.reset();
  }
  callback_.Run(result);
  delete this;
}

// If the initialization of the cache fails, and |force| is true, we will
// discard the whole cache and create a new one.
void CacheCreator::OnIOComplete(int result) {
  if (result == net::OK || !force_ || retry_)
    return DoCallback(result);

  // This is a failure and we are supposed to try again, so delete the object,
  // delete all the files, and try again.
  retry_ = true;
  created_cache_.reset();
  if (!disk_cache::DelayedCacheCleanup(path_))
    return DoCallback(result);

  // The worker thread will start deleting files soon, but the original folder
  // is not there anymore... let's create a new set of files.
  int rv = Run();
  DCHECK_EQ(net::ERR_IO_PENDING, rv);
}

}  // namespace

namespace disk_cache {

int CreateCacheBackendImpl(
    net::CacheType type,
    net::BackendType backend_type,
    const base::FilePath& path,
    int max_bytes,
    bool force,
    const scoped_refptr<base::SingleThreadTaskRunner>& thread,
    net::NetLog* net_log,
    std::unique_ptr<Backend>* backend,
    disk_cache::PostCleanupCallback post_cleanup_callback,
    const net::CompletionCallback& callback) {
  DCHECK(!callback.is_null());

  if (type == net::MEMORY_CACHE) {
    *backend = disk_cache::MemBackendImpl::CreateBackend(max_bytes, net_log);
    return *backend ? net::OK : net::ERR_FAILED;
  }

  CacheCreator* creator = new CacheCreator(
      path, force, max_bytes, type, backend_type, kNone, thread, net_log,
      backend, std::move(post_cleanup_callback), callback);

  return creator->TryMakeCleanupContextAndRun();
}

int CreateCacheBackend(
    net::CacheType type,
    net::BackendType backend_type,
    const base::FilePath& path,
    int max_bytes,
    bool force,
    const scoped_refptr<base::SingleThreadTaskRunner>& thread,
    net::NetLog* net_log,
    std::unique_ptr<Backend>* backend,
    const net::CompletionCallback& callback) {
  return CreateCacheBackendImpl(type, backend_type, path, max_bytes, force,
                                thread, net_log, backend, PostCleanupCallback(),
                                callback);
}

int CreateCacheBackend(net::CacheType type,
                       net::BackendType backend_type,
                       const base::FilePath& path,
                       int max_bytes,
                       bool force,
                       net::NetLog* net_log,
                       std::unique_ptr<Backend>* backend,
                       const net::CompletionCallback& callback) {
  return CreateCacheBackendImpl(type, backend_type, path, max_bytes, force,
                                nullptr, net_log, backend,
                                PostCleanupCallback(), callback);
}

int CreateCacheBackend(net::CacheType type,
                       net::BackendType backend_type,
                       const base::FilePath& path,
                       int max_bytes,
                       bool force,
                       net::NetLog* net_log,
                       std::unique_ptr<Backend>* backend,
                       PostCleanupCallback post_cleanup_callback,
                       const net::CompletionCallback& callback) {
  return CreateCacheBackendImpl(type, backend_type, path, max_bytes, force,
                                nullptr, net_log, backend,
                                std::move(post_cleanup_callback), callback);
}

void FlushCacheThreadForTesting() {
  // For simple backend.
  SimpleBackendImpl::FlushWorkerPoolForTesting();
  base::TaskScheduler::GetInstance()->FlushForTesting();

  // Block backend.
  BackendImpl::FlushForTesting();
}

int Backend::CalculateSizeOfEntriesBetween(base::Time initial_time,
                                           base::Time end_time,
                                           const CompletionCallback& callback) {
  return net::ERR_NOT_IMPLEMENTED;
}

}  // namespace disk_cache
