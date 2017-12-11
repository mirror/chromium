// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/utility/ash_and_ui_service.h"

#include <memory>
#include <utility>

#include "ash/public/interfaces/constants.mojom.h"
#include "ash/window_manager_service.h"
#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/service_manager/public/interfaces/service_factory.mojom.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "services/ui/service.h"

namespace {

class ServiceFactoryImpl : public service_manager::mojom::ServiceFactory {
 public:
  ServiceFactoryImpl() = default;
  ~ServiceFactoryImpl() override = default;

  // service_manager::mojom::ServiceFactory:
  void CreateService(service_manager::mojom::ServiceRequest request,
                     const std::string& name) override {
    // JAMES does this need an embedded service runner?
    LOG(ERROR) << "JAMES CreateService " << name;
    if (name == ash::mojom::kServiceName) {
      window_manager_service_ = std::make_unique<ash::WindowManagerService>(
          true /* show_primary_host_on_connect */);
    } else if (name == ui::mojom::kServiceName) {
      ui_service_ = std::make_unique<ui::Service>();
    } else {
      NOTREACHED();
    }
  }

 private:
  // JAMES these probably need better lifetime management -- they just get
  // torn down when the strongbinding dies
  std::unique_ptr<ash::WindowManagerService> window_manager_service_;
  std::unique_ptr<ui::Service> ui_service_;

  DISALLOW_COPY_AND_ASSIGN(ServiceFactoryImpl);
};

void BindServiceFactoryRequestOnMainThread(
    service_manager::mojom::ServiceFactoryRequest request) {
  mojo::MakeStrongBinding(std::make_unique<ServiceFactoryImpl>(),
                          std::move(request));
}

}  // namespace

AshAndUiService::AshAndUiService() = default;

AshAndUiService::~AshAndUiService() = default;

void AshAndUiService::OnStart() {
  LOG(ERROR) << "JAMES OnStart";
  registry_.AddInterface(base::Bind(&BindServiceFactoryRequestOnMainThread),
                         base::ThreadTaskRunnerHandle::Get());
}

void AshAndUiService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  LOG(ERROR) << "JAMES OnBindInterface " << interface_name;
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}
