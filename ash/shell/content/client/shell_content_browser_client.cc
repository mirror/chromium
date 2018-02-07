// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shell/content/client/shell_content_browser_client.h"

#include <utility>

#include "ash/shell/content/client/shell_browser_main_parts.h"
#include "ash/shell/grit/ash_shell_resources.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/json/json_reader.h"
#include "base/strings/utf_string_conversions.h"
#include "components/font_service/public/interfaces/constants.mojom.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/service_names.mojom.h"
#include "content/public/utility/content_utility_client.h"
#include "services/service_manager/public/cpp/service.h"
#include "storage/browser/quota/quota_settings.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/aura/window.h"
#include "ui/base/resource/resource_bundle.h"

#include "services/ui/public/interfaces/window_manager.mojom.h"
#include "services/ui/public/interfaces/window_tree.mojom.h"
#include "services/ui/ws2/window_service.h"
#include "services/ui/ws2/window_service_delegate.h"
#include "services/ui/ws2/window_tree_factory.h"

#include "ash/wm/container_finder.h"
#include "content/browser/gpu/gpu_client.h"
#include "content/common/child_process_host_impl.h"
#include "ui/aura/client/window_types.h"
#include "ui/compositor/layer_type.h"

namespace ash {
namespace shell {
namespace {

class WindowServiceDelegateImpl : public ui::ws2::WindowServiceDelegate {
 public:
  WindowServiceDelegateImpl() {}
  ~WindowServiceDelegateImpl() override = default;

  void ClientRequestedWindowBoundsChange(aura::Window* window,
                                         const gfx::Rect& bounds) override {
    window->SetBounds(bounds);
  }

  std::unique_ptr<aura::Window> NewTopLevel(
      const std::unordered_map<std::string, std::vector<uint8_t>>& properties)
      override {
    std::unique_ptr<aura::Window> window =
        std::make_unique<aura::Window>(nullptr);
    window->SetType(aura::client::WINDOW_TYPE_NORMAL);
    window->Init(ui::LAYER_NOT_DRAWN);
    ash::wm::GetDefaultParent(window.get(), gfx::Rect())
        ->AddChild(window.get());
    return window;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(WindowServiceDelegateImpl);
};

void BindWindowTreeFactoryRequest(ui::mojom::WindowTreeFactoryRequest request) {
  // XXX hack!
  static ui::ws2::WindowService* ws = nullptr;
  static ui::ws2::WindowTreeFactory* window_tree_factory = nullptr;
  if (!ws) {
    ws = new ui::ws2::WindowService(new WindowServiceDelegateImpl);
    window_tree_factory = new ui::ws2::WindowTreeFactory(ws);
  }
  window_tree_factory->AddBinding(std::move(request));
}

void BindGpuRequest(ui::mojom::GpuRequest request) {
  // XXX hack!
  LOG(ERROR) << "BIND GPU REQUEST!";
  content::GpuClient* gpu_client = new content::GpuClient(
      content::ChildProcessHostImpl::GenerateChildProcessUniqueId());
  gpu_client->Add(std::move(request));
}

class AshService : public service_manager::Service {
 public:
  explicit AshService(
      scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner)
      : main_thread_task_runner_(main_thread_task_runner) {}
  ~AshService() override = default;

  void OnStart() override {
    LOG(ERROR) << "StartAsh";
    registry_.AddInterface(base::BindRepeating(&BindWindowTreeFactoryRequest),
                           main_thread_task_runner_);
    registry_.AddInterface(base::BindRepeating(&BindGpuRequest));
  }

  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {
    registry_.BindInterface(interface_name, std::move(interface_pipe));
  }

 private:
  scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner_;
  service_manager::BinderRegistry registry_;

  DISALLOW_COPY_AND_ASSIGN(AshService);
};

std::unique_ptr<service_manager::Service> CreateAshService(
    scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner) {
  return std::make_unique<AshService>(std::move(main_thread_task_runner));
}

}  // namespace

ShellContentBrowserClient::ShellContentBrowserClient()
    : shell_browser_main_parts_(nullptr) {}

ShellContentBrowserClient::~ShellContentBrowserClient() = default;

content::BrowserMainParts* ShellContentBrowserClient::CreateBrowserMainParts(
    const content::MainFunctionParams& parameters) {
  shell_browser_main_parts_ = new ShellBrowserMainParts(parameters);
  return shell_browser_main_parts_;
}

void ShellContentBrowserClient::GetQuotaSettings(
    content::BrowserContext* context,
    content::StoragePartition* partition,
    storage::OptionalQuotaSettingsCallback callback) {
  storage::GetNominalDynamicSettings(
      partition->GetPath(), context->IsOffTheRecord(), std::move(callback));
}

std::unique_ptr<base::Value>
ShellContentBrowserClient::GetServiceManifestOverlay(base::StringPiece name) {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  int id = -1;
  if (name == content::mojom::kPackagedServicesServiceName)
    id = IDR_ASH_SHELL_CONTENT_PACKAGED_SERVICES_MANIFEST_OVERLAY;
  if (id == -1)
    return nullptr;

  base::StringPiece manifest_contents =
      rb.GetRawDataResourceForScale(id, ui::ScaleFactor::SCALE_FACTOR_NONE);
  return base::JSONReader::Read(manifest_contents);
}

std::vector<content::ContentBrowserClient::ServiceManifestInfo>
ShellContentBrowserClient::GetExtraServiceManifests() {
  return {
      {"quick_launch", IDR_ASH_SHELL_QUICK_LAUNCH_MANIFEST},
      {font_service::mojom::kServiceName, IDR_ASH_SHELL_FONT_SERVICE_MANIFEST}};
}

void ShellContentBrowserClient::RegisterOutOfProcessServices(
    OutOfProcessServiceMap* services) {
  (*services)["quick_launch"] =
      OutOfProcessServiceInfo(base::ASCIIToUTF16("quick_launch"));
  (*services)[font_service::mojom::kServiceName] = OutOfProcessServiceInfo(
      base::ASCIIToUTF16(font_service::mojom::kServiceName));
}

void ShellContentBrowserClient::RegisterInProcessServices(
    StaticServiceMap* services) {
  (*services)["ash"].factory = base::BindRepeating(
      &CreateAshService, base::ThreadTaskRunnerHandle::Get());
}

}  // namespace examples
}  // namespace views
