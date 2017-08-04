// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/media_service.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "media/base/media_log.h"
#include "media/cdm/cdm_module.h"
#include "media/mojo/services/interface_factory_impl.h"
#include "media/mojo/services/mojo_media_client.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/service_manager/public/cpp/connector.h"

#include "base/files/scoped_temp_dir.h"

namespace media {

// TODO(xhwang): Hook up MediaLog when possible.
MediaService::MediaService(std::unique_ptr<MojoMediaClient> mojo_media_client)
    : mojo_media_client_(std::move(mojo_media_client)) {
  DCHECK(mojo_media_client_);
  registry_.AddInterface<mojom::MediaService>(
      base::Bind(&MediaService::Create, base::Unretained(this)));
}

MediaService::~MediaService() {}

void MediaService::OnStart() {
  ref_factory_.reset(new service_manager::ServiceContextRefFactory(
      base::Bind(&service_manager::ServiceContext::RequestQuit,
                 base::Unretained(context()))));
  mojo_media_client_->Initialize(context()->connector());
}

void MediaService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

bool MediaService::OnServiceManagerConnectionLost() {
  interface_factory_bindings_.CloseAllBindings();
  mojo_media_client_.reset();
  return true;
}

void MediaService::Create(mojom::MediaServiceRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void MediaService::PreloadCdm(const base::FilePath& cdm_path) {
  // This will preload the CDM.
  // TODO(xhwang): Pass |cdm_path| to CdmModule to load the CDM.
  // Also let MediaService own the CdmModule.
  CdmModule::GetInstance();

  // This will trigger the sandbox to be sealed.
  mojo_media_client_->OnCdmPreloaded();

  // Test the sandbox is initialized.
  // TODO(xhwang): Remove this before commit.
  base::ScopedTempDir temp_dir;
  bool success = temp_dir.CreateUniqueTempDir();
  LOG(ERROR) << "CreateUniqueTempDir: " << success;
  base::FilePath file_path = temp_dir.GetPath().AppendASCII("read_write_file");
  base::File file(file_path, base::File::FLAG_CREATE | base::File::FLAG_READ |
                                 base::File::FLAG_WRITE);
  LOG(ERROR) << "file valid? " << file.IsValid();
}

void MediaService::CreateInterfaceFactory(
    mojom::InterfaceFactoryRequest request,
    service_manager::mojom::InterfaceProviderPtr host_interfaces) {
  // Ignore request if service has already stopped.
  if (!mojo_media_client_)
    return;

  interface_factory_bindings_.AddBinding(
      base::MakeUnique<InterfaceFactoryImpl>(
          std::move(host_interfaces), &media_log_, ref_factory_->CreateRef(),
          mojo_media_client_.get()),
      std::move(request));
}

}  // namespace media
