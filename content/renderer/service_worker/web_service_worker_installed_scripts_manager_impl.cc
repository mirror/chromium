// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/service_worker/web_service_worker_installed_scripts_manager_impl.h"

#include "base/synchronization/waitable_event.h"

namespace content {

namespace {

void InitializeOnIO(
    WebServiceWorkerInstalledScriptsManagerImpl* installed_scripts_manager,
    base::WaitableEvent* event) {
  installed_scripts_manager->InitializeOnIO();
  event->Signal();
}
}

// static
std::unique_ptr<blink::WebServiceWorkerInstalledScriptsManager>
WebServiceWorkerInstalledScriptsManagerImpl::Create(
    base::SingleThreadTaskRunner* io_task_runner) {
  auto installed_scripts_manager =
      base::WrapUnique<WebServiceWorkerInstalledScriptsManagerImpl>(
          new WebServiceWorkerInstalledScriptsManagerImpl(io_task_runner));
  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  io_task_runner->PostTask(
      FROM_HERE, base::BindOnce(&content::InitializeOnIO,
                                installed_scripts_manager.get(), &event));
  event.Wait();
  return installed_scripts_manager;
}

WebServiceWorkerInstalledScriptsManagerImpl::
    WebServiceWorkerInstalledScriptsManagerImpl(
        base::SingleThreadTaskRunner* io_task_runner)
    : io_task_runner_(io_task_runner) {}

WebServiceWorkerInstalledScriptsManagerImpl::
    ~WebServiceWorkerInstalledScriptsManagerImpl() {}

std::unique_ptr<blink::WebServiceWorkerInstalledScriptsManager::RawScriptData>
WebServiceWorkerInstalledScriptsManagerImpl::GetRawScriptData(
    const blink::WebURL& script_url) {
  auto contents = base::MakeUnique<RawScriptData>();
  if (meta_data.find(script_url) == meta_data.end()) {
    // Wait for arrival of the script.
    return nullptr;
  }
  // Ask to the browser process when the script has already been served.
  if (!meta_data[script_url])
    return nullptr;
  return std::move(meta_data[script_url]);
}

void WebServiceWorkerInstalledScriptsManagerImpl::TransferInstalledScript(
    const GURL& script_url,
    uint32_t body_size,
    mojo::ScopedDataPipeConsumerHandle body,
    uint32_t meta_data_size,
    mojo::ScopedDataPipeConsumerHandle meta_data) {}

}  // namespace content
