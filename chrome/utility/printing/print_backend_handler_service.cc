// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/utility/printing/print_backend_handler_service.h"

#include "build/build_config.h"
#include "chrome/utility/printing/print_backend_handler_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace printing {

namespace {

void OnPrintBackendHandlerRequest(
    service_manager::ServiceContextRefFactory* ref_factory,
    printing::mojom::PrintBackendHandlerRequest request) {
  mojo::MakeStrongBinding(std::make_unique<printing::PrintBackendHandlerImpl>(
                              ref_factory->CreateRef()),
                          std::move(request));
}

}  // namespace

PrintBackendHandlerService::PrintBackendHandlerService() {}

PrintBackendHandlerService::~PrintBackendHandlerService() {}

std::unique_ptr<service_manager::Service>
PrintBackendHandlerService::CreateService() {
  return base::MakeUnique<PrintBackendHandlerService>();
}

void PrintBackendHandlerService::OnStart() {
  ref_factory_ = std::make_unique<service_manager::ServiceContextRefFactory>(
      base::Bind(&service_manager::ServiceContext::RequestQuit,
                 base::Unretained(context())));
  registry_.AddInterface(
      base::Bind(&OnPrintBackendHandlerRequest, ref_factory_.get()));
}

void PrintBackendHandlerService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

}  // namespace printing
