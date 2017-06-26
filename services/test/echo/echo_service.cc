// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/test/echo/echo_service.h"

#include "services/service_manager/public/cpp/service_context.h"

namespace echo {

std::unique_ptr<service_manager::Service> CreateEchoService() {
  return base::MakeUnique<EchoService>();
}

EchoService::EchoService() {
  LOG(INFO) << "Echo Service created";
  registry_.AddInterface<mojom::Echo>(
      base::Bind(&EchoService::BindEchoRequest, base::Unretained(this)));
}

EchoService::~EchoService() {
  LOG(INFO) << "Echo Service dying";
}

void EchoService::OnStart() {}

void EchoService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  LOG(INFO) << "Echo Service OnBindInterface() for " << interface_name;
  registry_.BindInterface(source_info, interface_name,
                          std::move(interface_pipe));
}

void EchoService::BindEchoRequest(
    const service_manager::BindSourceInfo& source_info,
    mojom::EchoRequest request) {
  LOG(INFO) << "Echo Service BindEchoRequest";
  bindings_.AddBinding(this, std::move(request));
}

void EchoService::EchoString(const std::string& input,
                             EchoStringCallback callback) {
  LOG(INFO) << "EchoString";
  std::move(callback).Run(input);
  LOG(INFO) << "EchoString ran callback";
}

}  // namespace echo
