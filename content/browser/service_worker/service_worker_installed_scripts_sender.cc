// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_installed_scripts_sender.h"

namespace content {

ServiceWorkerInstalledScriptsSender::ServiceWorkerInstalledScriptsSender() {}

ServiceWorkerInstalledScriptsSender::~ServiceWorkerInstalledScriptsSender() {}

mojom::ServiceWorkerInstalledScriptsInfoPtr
ServiceWorkerInstalledScriptsSender::CreateAndBind() {
  auto info = mojom::ServiceWorkerInstalledScriptsInfo::New();
  info->manager_request = mojo::MakeRequest(&manager_);
  info->installed_urls.push_back(GURL("https://example.com/installed_hoge"));
  return info;
}

}  // namespace content
