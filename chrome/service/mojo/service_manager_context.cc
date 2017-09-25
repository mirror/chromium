// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/service/mojo/service_manager_context.h"

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/json/json_reader.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/process/process_handle.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "build/build_config.h"
#include "chrome/common/printing/printer_capabilities.mojom.h"
#include "chrome/grit/browser_resources.h"
#include "chrome/service/mojo/service_names.mojom.h"
#include "content/common/service_manager/child_connection.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/utility_process_host.h"
#include "content/public/browser/utility_process_host_client.h"
#include "content/public/common/child_process_host.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_descriptors.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/mojo_channel_switches.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/common/service_names.mojom.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/incoming_broker_client_invitation.h"
#include "mojo/edk/embedder/platform_channel_pair.h"
#include "mojo/edk/embedder/transport_protocol.h"
#include "services/catalog/manifest_provider.h"
#include "services/catalog/public/cpp/manifest_parsing_util.h"
#include "services/catalog/public/interfaces/constants.mojom.h"
#include "services/service_manager/connect_params.h"
#include "services/service_manager/embedder/manifest_utils.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/interfaces/service.mojom.h"
#include "services/service_manager/runner/common/client_util.h"
#include "services/service_manager/runner/common/switches.h"
#include "services/service_manager/runner/host/service_process_launcher.h"
#include "services/service_manager/service_manager.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_switches.h"

using content::ServiceManagerConnection;

namespace {

service_manager::Connector* g_io_thread_connector;

bool IsOnTaskRunner(base::SequencedTaskRunner* task_runner) {
  return base::MessageLoop::current()->task_runner() == task_runner;
}

// Launch a process for a service once its sandbox type is known.
void StartServiceInUtilityProcessNoSandbox(
    const service_manager::Identity& child_identity,
    scoped_refptr<base::SequencedTaskRunner> io_task_runner,
    const std::string& service_name,
    const base::string16& process_name,
    service_manager::mojom::ServiceRequest request,
    service_manager::mojom::ConnectResult query_result,
    const std::string& sandbox_string) {
  DCHECK(IsOnTaskRunner(io_task_runner.get()));
  LOG(ERROR) << "** JAY ** StartServiceInUtilityProcess";
  if (query_result != service_manager::mojom::ConnectResult::SUCCEEDED) {
    LOG(ERROR) << "Failed to start utility process.";
    return;
  }

  base::FilePath exe_path = content::ChildProcessHost::GetChildPath(
      content::ChildProcessHost::CHILD_NORMAL);
  if (exe_path.empty()) {
    NOTREACHED() << "Unable to get utility process binary name.";
    return;
  }

  base::CommandLine cmd_line(exe_path);
  cmd_line.AppendSwitchASCII(switches::kProcessType, switches::kUtilityProcess);
  cmd_line.AppendSwitch(switches::kLang);
#if defined(OS_WIN)
  cmd_line.AppendArg(switches::kPrefetchArgumentOther);
#endif
  cmd_line.AppendSwitch(switches::kNoSandbox);

  // Connect Mojo at the EDK level.
  mojo::edk::PlatformChannelPair edk_channels;
  mojo::edk::HandlePassingInformation handles_passing_info;

  // edk_channels.PrepareToPassClientHandleToChildProcess(&cmd_line,
  //     &handles_passing_info);
  mojo::edk::OutgoingBrokerClientInvitation invitation;

  // ChildConnection connects at the ServiceManager level.
  content::ChildConnection child_connection(
      child_identity, &invitation, g_io_thread_connector, io_task_runner);
  cmd_line.AppendSwitchASCII(switches::kServiceRequestChannelToken,
                             child_connection.service_token());

  base::LaunchOptions launch_options;
#if defined(OS_WIN)
  launch_options.handles_to_inherit = handles_passing_info;
#else
  mojo::edk::ScopedPlatformHandle edk_client_handle =
      edk_channels.PassClientHandle();
  launch_options.fds_to_remap.push_back(
      std::make_pair(edk_client_handle.get().handle, kMojoIPCChannel));
#endif
  base::Process process = base::LaunchProcess(cmd_line, launch_options);
  if (!process.IsValid()) {
    LOG(ERROR) << "Failed to launch utility process";
    return;
  }
  child_connection.SetProcessHandle(process.Handle());

  invitation.Send(process.Handle(), mojo::edk::ConnectionParams(
                                        mojo::edk::TransportProtocol::kLegacy,
                                        edk_channels.PassServerHandle()));

  service_manager::mojom::ServiceFactoryPtr service_factory;
  child_connection.BindInterface(
      service_manager::mojom::ServiceFactory::Name_,
      mojo::MakeRequest(&service_factory).PassMessagePipe());
  service_factory->CreateService(std::move(request), service_name);
}

// Determine a sandbox type for a service and launch a process for it.
void QueryAndStartServiceInUtilityProcessNoSandbox(
    scoped_refptr<base::SequencedTaskRunner> io_task_runner,
    const std::string& service_name,
    const base::string16& process_name,
    service_manager::mojom::ServiceRequest request) {
  LOG(ERROR) << "** JAY ** QueryAndStartServiceInUtilityProcessNoSandbox";
  DCHECK(IsOnTaskRunner(io_task_runner.get()));

  service_manager::Identity identity(service_name);
  ServiceManagerContext::GetConnectorForIOThread()->QueryService(
      identity, base::BindOnce(&StartServiceInUtilityProcessNoSandbox, identity,
                               io_task_runner, service_name, process_name,
                               std::move(request)));
}

// A ManifestProvider which resolves application names to builtin manifest
// resources for the catalog service to consume.
class BuiltinManifestProvider : public catalog::ManifestProvider {
 public:
  BuiltinManifestProvider() {}
  ~BuiltinManifestProvider() override {}

  void AddServiceManifest(base::StringPiece name, int resource_id) {
    std::string contents =
        ui::ResourceBundle::GetSharedInstance()
            .GetRawDataResourceForScale(resource_id,
                                        ui::ScaleFactor::SCALE_FACTOR_NONE)
            .as_string();
    DCHECK(!contents.empty());
    LOG(ERROR) << "** JAY ** AddServiceManifest " << name;

    std::unique_ptr<base::Value> manifest_value =
        base::JSONReader::Read(contents);
    DCHECK(manifest_value);

    // ** TODO ** Pass these files when creating the process.
    // base::Optional<catalog::RequiredFileMap> required_files =
    //     catalog::RetrieveRequiredFiles(*manifest_value);
    // if (required_files) {
    //   content::ChildProcessLauncher::SetRegisteredFilesForService(
    //       name.as_string(), std::move(*required_files));
    // }

    auto result = manifests_.insert(
        std::make_pair(name.as_string(), std::move(manifest_value)));
    DCHECK(result.second) << "Duplicate manifest entry: " << name;
  }

 private:
  // catalog::ManifestProvider:
  std::unique_ptr<base::Value> GetManifest(const std::string& name) override {
    auto it = manifests_.find(name);
    LOG(ERROR) << "** JAY ** GetManifest " << name << " found? "
               << (it != manifests_.end());
    return it != manifests_.end() ? it->second->CreateDeepCopy() : nullptr;
  }

  std::map<std::string, std::unique_ptr<base::Value>> manifests_;

  DISALLOW_COPY_AND_ASSIGN(BuiltinManifestProvider);
};

class NullServiceProcessLauncherFactory
    : public service_manager::ServiceProcessLauncherFactory {
 public:
  NullServiceProcessLauncherFactory() {}
  ~NullServiceProcessLauncherFactory() override {}

 private:
  std::unique_ptr<service_manager::ServiceProcessLauncher> Create(
      const base::FilePath& service_path) override {
    LOG(ERROR) << "Attempting to run unsupported native service: "
               << service_path.value();
    return nullptr;
  }

  DISALLOW_COPY_AND_ASSIGN(NullServiceProcessLauncherFactory);
};

}  // namespace

// State which lives on the IO thread and drives the ServiceManager.
class ServiceManagerContext::InProcessServiceManagerContext
    : public base::RefCountedThreadSafe<InProcessServiceManagerContext> {
 public:
  explicit InProcessServiceManagerContext(
      scoped_refptr<base::SequencedTaskRunner> io_task_runner)
      : io_task_runner_(io_task_runner) {}

  void Start(
      service_manager::mojom::ServicePtrInfo packaged_services_service_info,
      std::unique_ptr<BuiltinManifestProvider> manifest_provider) {
    io_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&InProcessServiceManagerContext::StartOnIOThread, this,
                       base::Passed(&manifest_provider),
                       base::Passed(&packaged_services_service_info)));
  }

  void ShutDown() {
    io_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&InProcessServiceManagerContext::ShutDownOnIOThread,
                       this));
  }

 private:
  friend class base::RefCountedThreadSafe<InProcessServiceManagerContext>;

  ~InProcessServiceManagerContext() {}

  void StartOnIOThread(
      std::unique_ptr<BuiltinManifestProvider> manifest_provider,
      service_manager::mojom::ServicePtrInfo packaged_services_service_info) {
    LOG(ERROR) << "** JAY ** InProcessServiceManagerContext::StartOnIOThread";
    manifest_provider_ = std::move(manifest_provider);
    service_manager_ = std::make_unique<service_manager::ServiceManager>(
        std::make_unique<NullServiceProcessLauncherFactory>(), nullptr,
        manifest_provider_.get());

    service_manager::mojom::ServicePtr packaged_services_service;
    packaged_services_service.Bind(std::move(packaged_services_service_info));
    service_manager_->RegisterService(
        service_manager::Identity(chrome::mojom::kPackagedServicesServiceName,
                                  service_manager::mojom::kRootUserID),
        std::move(packaged_services_service), nullptr);
  }

  void ShutDownOnIOThread() {
    service_manager_.reset();
    manifest_provider_.reset();
  }

  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;
  std::unique_ptr<BuiltinManifestProvider> manifest_provider_;
  std::unique_ptr<service_manager::ServiceManager> service_manager_;

  DISALLOW_COPY_AND_ASSIGN(InProcessServiceManagerContext);
};

ServiceManagerContext::ServiceManagerContext(
    scoped_refptr<base::SequencedTaskRunner> io_task_runner)
    : io_task_runner_(io_task_runner) {
  in_process_context_ = new InProcessServiceManagerContext(io_task_runner);

  auto manifest_provider = std::make_unique<BuiltinManifestProvider>();
  manifest_provider->AddServiceManifest(
      chrome::mojom::kPackagedServicesServiceName, IDR_CHROME_SERVICE_MANIFEST);

  service_manager::mojom::ServicePtr packaged_services_service;
  service_manager::mojom::ServiceRequest packaged_services_request =
      mojo::MakeRequest(&packaged_services_service);
  in_process_context_->Start(packaged_services_service.PassInterface(),
                             std::move(manifest_provider));
  packaged_services_connection_ = ServiceManagerConnection::Create(
      std::move(packaged_services_request), io_task_runner_);

  packaged_services_connection_->AddServiceRequestHandler(
      printing::mojom::kPrinterCapabilitiesServiceName,
      base::Bind(&QueryAndStartServiceInUtilityProcessNoSandbox, io_task_runner,
                 printing::mojom::kPrinterCapabilitiesServiceName,
                 base::ASCIIToUTF16("Printer Capabilities")));
  packaged_services_connection_->Start();

  DCHECK(!g_io_thread_connector);
  g_io_thread_connector =
      packaged_services_connection_->GetConnector()->Clone().release();
}

ServiceManagerContext::~ServiceManagerContext() {
  // NOTE: The in-process ServiceManager MUST be destroyed before the browser
  // process-wide ServiceManagerConnection. Otherwise it's possible for the
  // ServiceManager to receive connection requests for service:content_browser
  // which it may attempt to service by launching a new instance of the browser.
  if (in_process_context_)
    in_process_context_->ShutDown();
  delete g_io_thread_connector;
  g_io_thread_connector = nullptr;
}

bool ServiceManagerContext::IsOnIOThread() {
  return IsOnTaskRunner(io_task_runner_.get());
}

// static
service_manager::Connector* ServiceManagerContext::GetConnectorForIOThread() {
  return g_io_thread_connector;
}
