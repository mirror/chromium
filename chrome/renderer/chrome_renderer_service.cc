// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/chrome_renderer_service.h"

// static
std::unique_ptr<service_manager::Service> ChromeRendererService::Create(
    service_manager::BinderRegistry* registry) {
  return base::MakeUnique<ChromeRendererService>(registry);
}

ChromeRendererService::ChromeRendererService(
    service_manager::BinderRegistry* registry)
    : registry_(registry) {}

ChromeRendererService::~ChromeRendererService() {}

void ChromeRendererService::OnBindInterface(
    const service_manager::BindSourceInfo& remote_info,
    const std::string& name,
    mojo::ScopedMessagePipeHandle handle) {
  registry_->TryBindInterface(name, &handle);
}
