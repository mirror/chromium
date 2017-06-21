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
}  // namespace

// static
std::unique_ptr<blink::WebServiceWorkerInstalledScriptsManager>
WebServiceWorkerInstalledScriptsManagerImpl::Create(
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner) {
  auto installed_scripts_manager =
      base::WrapUnique<WebServiceWorkerInstalledScriptsManagerImpl>(
          new WebServiceWorkerInstalledScriptsManagerImpl(
              std::move(io_task_runner)));
  // TODO(shimazu): Don't use waitable event here.
  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  io_task_runner->PostTask(
      FROM_HERE, base::BindOnce(&content::InitializeOnIO,
                                installed_scripts_manager.get(), &event));
  event.Wait();
  return base::WrapUnique(installed_scripts_manager.release());
}

WebServiceWorkerInstalledScriptsManagerImpl::
    WebServiceWorkerInstalledScriptsManagerImpl(
        scoped_refptr<base::SingleThreadTaskRunner> io_task_runner)
    : io_task_runner_(std::move(io_task_runner)) {}

WebServiceWorkerInstalledScriptsManagerImpl::
    ~WebServiceWorkerInstalledScriptsManagerImpl() {}

void WebServiceWorkerInstalledScriptsManagerImpl::InitializeOnIO() {
  // TODO(shimazu): Bind Mojo's pipe here.
}

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
    mojo::ScopedDataPipeConsumerHandle meta_data) {
  // TODO(shimazu): implement here.
}

}  // namespace content
