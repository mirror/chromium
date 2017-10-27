// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/chrome_renderer_service.h"

#include "content/public/browser/browser_thread.h"

// static
std::unique_ptr<service_manager::Service> ChromeRendererService::Create() {
  return base::MakeUnique<ChromeRendererService>();
}

ChromeRendererService::ChromeRendererService() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
}

ChromeRendererService::~ChromeRendererService() {}

void ChromeRendererService::OnBindInterface(
    const service_manager::BindSourceInfo& remote_info,
    const std::string& name,
    mojo::ScopedMessagePipeHandle handle) {
  if (!registry_.TryBindInterface(name, &handle))
    registry_with_source_info_.TryBindInterface(name, &handle, remote_info);
}
