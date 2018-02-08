// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <utility>

#include "services/service_manager/public/c/main.h"
#include "chromeos/services/assistant/service.h"
#include "services/service_manager/public/cpp/service_runner.h"

MojoResult ServiceMain(MojoHandle service_request_handle) {
  auto assistant_delegate = std::make_unique<
      chromeos::assistant::Service::AssistantServiceDelegate>();
  service_manager::ServiceRunner runner(
      new chromeos::assistant::Service(assistant_delegate.get()));
  return runner.Run(service_request_handle);
}
