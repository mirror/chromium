// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/smb_client/smb_service.h"
#include "chrome/browser/chromeos/file_system_provider/service.h"
#include "chrome/browser/chromeos/smb_client/smb_file_system.h"

using chromeos::file_system_provider::Service;

namespace chromeos {
namespace smb_client {
namespace {

using file_system_provider::ProvidedFileSystemInterface;

// Factory for smb file systems. |profile| must not be NULL.
std::unique_ptr<ProvidedFileSystemInterface> CreateSambaFileSystem(
    Profile* profile,
    const file_system_provider::ProvidedFileSystemInfo& file_system_info) {
  return base::MakeUnique<SmbFileSystem>(file_system_info);
}

}  // namespace

SmbService::SmbService(Profile* profile) : profile_(profile) {
  file_system_provider::Service* const service =
      file_system_provider::Service::Get(profile_);

  service->RegisterFileSystemFactory("smb", base::Bind(&CreateSambaFileSystem));
}

SmbService::~SmbService() {}

base::File::Error SmbService::MountSmb(
    const file_system_provider::MountOptions& options) {
  file_system_provider::Service* const service =
      file_system_provider::Service::Get(profile_);

  return service->MountFileSystem("smb", options);
}

}  // namespace smb_client
}  // namespace chromeos
