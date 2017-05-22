// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/browser/layout_test/layout_test_helper_service.h"

#include <string>

namespace content {

LayoutTestHelperService::LayoutTestHelperService() {}

LayoutTestHelperService::~LayoutTestHelperService() {}

// static
std::unique_ptr<service_manager::Service> LayoutTestHelperService::Create() {
  return base::MakeUnique<LayoutTestHelperService>();
}

void LayoutTestHelperService::OnBindInterface(
    const service_manager::BindSourceInfo& source,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  if (interface_name == blink::mojom::LayoutTestHelper::Name_) {
    bindings_.AddBinding(
        this, blink::mojom::LayoutTestHelperRequest(std::move(interface_pipe)));
  }
}

void LayoutTestHelperService::Reflect(const std::string& message,
                                      ReflectCallback callback) {
  std::move(callback).Run(std::string(message.rbegin(), message.rend()));
}

}  // namespace content
