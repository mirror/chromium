// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>
#include <memory>

#include "base/memory/ref_counted.h"
#include "base/synchronization/condition_variable.h"
#include "base/synchronization/lock.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/WebServiceWorkerInstalledScriptsManager.h"

#ifndef CONTENT_RENDERER_SERVICE_WORKER_THREAD_SAFE_SCRIPT_CONTAINER_H_
#define CONTENT_RENDERER_SERVICE_WORKER_THREAD_SAFE_SCRIPT_CONTAINER_H_

namespace content {

// Container class accessible from any threads which stores a set of scripts for
// launching a service worker. The IO thread adds scripts to the container, and
// the worker thread takes the scripts. The worker thread should be blocked
// until the script is received from the browser process because importScripts
// needs to be served synchronously. This class is owned by
// WebServiceWorkerInstalledScriptsManagerImpl on the worker thread and its
// internal class on the IO thread.
class ThreadSafeScriptContainer
    : public base::RefCountedThreadSafe<ThreadSafeScriptContainer> {
 public:
  using Data = blink::WebServiceWorkerInstalledScriptsManager::RawScriptData;

  REQUIRE_ADOPTION_FOR_REFCOUNTED_TYPE();
  ThreadSafeScriptContainer();

  // Called on the IO thread.
  void Add(const GURL& url, std::unique_ptr<Data> data);
  // Called on the worker thread.
  bool Exists(const GURL& url);
  // Waits until the script is added. The thread is blocked until the script is
  // available. Only one thread can wait at a time. Returns false if an error
  // happens and the waiting script won't be available forever.
  // Called on the worker thread.
  bool Wait(const GURL& url);
  // Called on the worker thread.
  std::unique_ptr<Data> Take(const GURL& url);

  // Called if no more data will be added.
  void OnAllDataAdded();

 private:
  friend class base::RefCountedThreadSafe<ThreadSafeScriptContainer>;
  ~ThreadSafeScriptContainer();

  // |lock_| protects |waiting_cv_|, |script_data_|, |waiting_url_| and
  // |is_completed_|.
  base::Lock lock_;
  // |waiting_cv_| wakes up when a script whose url matches to |waiting_url| is
  // added, or OnAllDataAdded is called. This waits on the worker therad, and
  // signals on the IO thread.
  base::ConditionVariable waiting_cv_;
  std::map<GURL, std::unique_ptr<Data>> script_data_;
  GURL waiting_url_;
  bool are_all_data_added_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_SERVICE_WORKER_THREAD_SAFE_SCRIPT_CONTAINER_H_
