// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_SERVICE_WORKER_WEB_SERVICE_WORKER_INSTALLED_SCRIPTS_MANAGER_IMPL_H_
#define CONTENT_RENDERER_SERVICE_WORKER_WEB_SERVICE_WORKER_INSTALLED_SCRIPTS_MANAGER_IMPL_H_

#include <map>
#include <set>

#include "base/memory/ref_counted.h"
#include "base/synchronization/condition_variable.h"
#include "base/synchronization/lock.h"
#include "content/common/service_worker/service_worker_installed_scripts_manager.mojom.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/WebServiceWorkerInstalledScriptsManager.h"

namespace content {

class WebServiceWorkerInstalledScriptsManagerImpl final
    : NON_EXPORTED_BASE(public blink::WebServiceWorkerInstalledScriptsManager) {
 public:
  // Container class accessible from any threads.
  class ThreadSafeScriptContainer
      : public base::RefCountedThreadSafe<ThreadSafeScriptContainer> {
   public:
    REQUIRE_ADOPTION_FOR_REFCOUNTED_TYPE();
    ThreadSafeScriptContainer();

    void Add(const GURL& url, std::unique_ptr<RawScriptData> data);
    bool Exists(const GURL& url);
    // Waits until the script is added. Only one thread can wait at a time.
    // Returns false if error happens.
    bool Wait(const GURL& url);
    std::unique_ptr<RawScriptData> Take(const GURL& url);

    // Called if no more data will be added.
    void OnCompleted();

   private:
    friend class base::RefCountedThreadSafe<ThreadSafeScriptContainer>;
    ~ThreadSafeScriptContainer();

    std::map<GURL, std::unique_ptr<RawScriptData>> script_data_;
    base::Lock lock_;
    GURL waiting_url_;
    base::ConditionVariable waiting_cv_;
    bool is_completed;
  };

  // Called on the main thread.
  static std::unique_ptr<blink::WebServiceWorkerInstalledScriptsManager> Create(
      mojom::ServiceWorkerInstalledScriptsInfoPtr installed_scripts_info,
      scoped_refptr<base::SingleThreadTaskRunner> io_task_runner);

  ~WebServiceWorkerInstalledScriptsManagerImpl() override;

  // WebServiceWorkerInstalledScriptsManager implementation.
  bool IsScriptInstalled(const blink::WebURL& script_url) const override;
  std::unique_ptr<RawScriptData> GetRawScriptData(
      const blink::WebURL& script_url) override;

 private:
  explicit WebServiceWorkerInstalledScriptsManagerImpl(
      std::vector<GURL>&& installed_urls,
      scoped_refptr<ThreadSafeScriptContainer> script_container);

  const std::set<GURL> installed_urls_;
  scoped_refptr<ThreadSafeScriptContainer> script_container_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_SERVICE_WORKER_WEB_SERVICE_WORKER_INSTALLED_SCRIPTS_MANAGER_IMPL_H_
