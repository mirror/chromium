// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/smb_client/smb_service.h"
#include "chrome/browser/chromeos/file_system_provider/service.h"

using chromeos::file_system_provider::Service;

namespace chromeos {
namespace smb_client {
namespace {

// Factory for smb file systems to go here after other Cl lands

}  // namespace

base::File::Error SmbService::MountSmb(
    file_system_provider::MountOptions& options) {
  file_system_provider::Service* const service =
      file_system_provider::Service::Get(profile_);

  return service->MountFileSystem("smb", options);
}

}  // namespace smb_client
}  // namespace chromeos
