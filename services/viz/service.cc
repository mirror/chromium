// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/viz/service.h"

#include "components/viz/service/main/viz_main_impl.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/viz/privileged/interfaces/viz_main.mojom.h"

namespace viz {

Service::Service(std::string ukm_service_name)
    : ukm_service_name_(ukm_service_name) {}

Service::~Service() = default;

void Service::OnStart() {
  base::PlatformThread::SetName("VizMain");
  registry_.AddInterface<mojom::VizMain>(
      base::Bind(&Service::BindVizMainRequest, base::Unretained(this)));

  VizMainImpl::ExternalDependencies deps;
  deps.create_display_compositor = true;
  ukm::mojom::UkmRecorderInterfacePtr ukm_recorder_ptr;
  context()->connector()->BindInterface(ukm_service_name_,
                                        mojo::MakeRequest(&ukm_recorder_ptr));
  deps.ukm_recorder_ptr_info = ukm_recorder_ptr.PassInterface();
  viz_main_ = std::make_unique<VizMainImpl>(nullptr, std::move(deps));
}

void Service::OnBindInterface(const service_manager::BindSourceInfo& info,
                              const std::string& interface_name,
                              mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

void Service::BindVizMainRequest(mojom::VizMainRequest request) {
  viz_main_->Bind(std::move(request));
}

}  // namespace viz
