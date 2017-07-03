// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_installed_scripts_sender.h"

#include "base/trace_event/trace_event.h"
#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/service_worker/service_worker_disk_cache.h"
#include "content/browser/service_worker/service_worker_script_cache_map.h"
#include "content/browser/service_worker/service_worker_storage.h"
#include "net/base/io_buffer.h"
#include "net/http/http_response_headers.h"

namespace content {

ServiceWorkerInstalledScriptsSender::ServiceWorkerInstalledScriptsSender() =
    default;
ServiceWorkerInstalledScriptsSender::~ServiceWorkerInstalledScriptsSender() =
    default;

mojom::ServiceWorkerInstalledScriptsInfoPtr
ServiceWorkerInstalledScriptsSender::CreateInfoAndBind() {
  auto info = mojom::ServiceWorkerInstalledScriptsInfo::New();
  info->manager_request = mojo::MakeRequest(&manager_);

  // TODO(shimazu): Read all installed urls and start pushing scripts.
  return info;
}

}  // namespace content
