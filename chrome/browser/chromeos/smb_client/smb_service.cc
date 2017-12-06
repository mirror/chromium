// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/smb_client/smb_service.h"

#include "base/strings/string_number_conversions.h"
#include "chrome/browser/chromeos/file_system_provider/provided_file_system_info.h"
#include "chrome/browser/chromeos/smb_client/smb_file_system.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/smb_provider_client.h"

using chromeos::file_system_provider::Service;

namespace chromeos {
namespace smb_client {

file_system_provider::ProviderId kSmbProviderId =
    ProviderId::CreateFromNativeId("smb");

SmbService::SmbService(Profile* profile)
    : profile_(profile), weak_ptr_factory_(this) {
  GetProviderService()->RegisterNativeProvider(
      kSmbProviderId, std::make_unique<SmbService>(profile));
}

SmbService::~SmbService() {}

void SmbService::Mount(const file_system_provider::MountOptions& options,
                       const std::string& share_path,
                       MountResponse callback) {
  chromeos::DBusThreadManager::Get()->GetSmbProviderClient()->Mount(
      share_path, base::BindOnce(&SmbService::OnMountResponse,
                                 weak_ptr_factory_.GetWeakPtr(),
                                 base::Passed(&callback), options));
}

void SmbService::OnMountResponse(
    MountResponse callback,
    const file_system_provider::MountOptions& options,
    smbprovider::ErrorType error,
    int32_t mount_id) {
  if (error != smbprovider::ERROR_OK) {
    std::move(callback).Run(std::move(SmbFileSystem::TranslateError(error)));
  }

  file_system_provider::MountOptions mount_options(options);
  mount_options.file_system_id = base::NumberToString(mount_id);

  base::File::Error providerServiceMountResult =
      GetProviderService()->MountFileSystem(kSmbProviderId, mount_options);

  std::move(callback).Run(std::move(providerServiceMountResult));
}

Service* SmbService::GetProviderService() const {
  return file_system_provider::Service::Get(profile_);
}

std::unique_ptr<ProvidedFileSystemInterface>
SmbService::CreateProvidedFileSystem(
    Profile* profile,
    const ProvidedFileSystemInfo& file_system_info) {
  DCHECK(profile);
  return std::make_unique<SmbFileSystem>(file_system_info);
}

bool SmbService::GetCapabilities(Profile* profile,
                                 const ProviderId& provider_id,
                                 Capabilities& result) {
  result = Capabilities(false, false, false, extensions::SOURCE_NETWORK);
  return true;
}

}  // namespace smb_client
}  // namespace chromeos
