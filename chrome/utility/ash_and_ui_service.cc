// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/utility/ash_and_ui_service.h"

#include <memory>
#include <utility>

#include "ash/public/interfaces/constants.mojom.h"
#include "ash/window_manager_service.h"
#include "base/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/threading/thread_task_runner_handle.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/service_manager/embedder/embedded_service_info.h"
#include "services/service_manager/embedder/embedded_service_runner.h"
#include "services/service_manager/public/interfaces/service_factory.mojom.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "services/ui/service.h"

using service_manager::EmbeddedServiceRunner;

namespace {

std::unique_ptr<service_manager::Service> CreateAshService() {
  LOG(ERROR) << "JAMES CreateAshService";
  const bool show_primary_host_on_connect = true;
  return std::make_unique<ash::WindowManagerService>(
      show_primary_host_on_connect);
}

//JAMES should this clone CreateEmbeddedUIService?
//JAMES this is on a different thread than OnStart() below.
std::unique_ptr<service_manager::Service> CreateUiService(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner) {
  LOG(ERROR) << "JAMES CreateUiService";
  ui::Service::InProcessConfig config;
  config.resource_runner = task_runner;
  // config.image_cursors_set_weak_ptr = image_cursors_set_weak_ptr;
  // config.memory_manager = memory_manager;
  config.should_host_viz = true;  // In mash, ash/mus always hosts viz.
  return std::make_unique<ui::Service>(&config);
}

class ServiceFactoryImpl : public service_manager::mojom::ServiceFactory {
 public:
  ServiceFactoryImpl() = default;
  ~ServiceFactoryImpl() override = default;

  // service_manager::mojom::ServiceFactory:
  void CreateService(service_manager::mojom::ServiceRequest request,
                     const std::string& name) override {
    LOG(ERROR) << "JAMES CreateService " << name;
    if (name == ash::mojom::kServiceName) {
      service_manager::EmbeddedServiceInfo info;
      info.factory = base::BindRepeating(&CreateAshService);
      ash_runner_ = std::make_unique<EmbeddedServiceRunner>(name, info);
      ash_runner_->BindServiceRequest(std::move(request));
    // TODO(jamescook): Should this SetQuitClosure()?
      return;
    }

    // SEE RegisterUIServiceInProcessIfNecessary in service_manager_context.cc
    //JAMES what thread are we on here?
    if (name == ui::mojom::kServiceName) {
      service_manager::EmbeddedServiceInfo info;
      info.factory = base::BindRepeating(&CreateUiService,
          base::ThreadTaskRunnerHandle::Get());
      //JAMES is this needed?
      info.use_own_thread = true;
      info.message_loop_type = base::MessageLoop::TYPE_UI;
      info.thread_priority = base::ThreadPriority::DISPLAY;
      ui_runner_ = std::make_unique<EmbeddedServiceRunner>(name, info);
      ui_runner_->BindServiceRequest(std::move(request));
      // TODO(jamescook): Should this SetQuitClosure()?
      return;
    }
    NOTREACHED() << "Unknown service " << name;
  }

 private:
  // JAMES these probably need better lifetime management -- they just get
  // torn down when the strongbinding dies
  // std::unique_ptr<ash::WindowManagerService> window_manager_service_;
  // std::unique_ptr<ui::Service> ui_service_;
  std::unique_ptr<EmbeddedServiceRunner> ash_runner_;
  std::unique_ptr<EmbeddedServiceRunner> ui_runner_;

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
