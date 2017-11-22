// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/file_system_provider/extension_provider.h"

#include <stddef.h>

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "chrome/browser/chromeos/file_system_provider/provided_file_system.h"
#include "chrome/browser/chromeos/file_system_provider/service.h"
#include "chrome/browser/chromeos/file_system_provider/throttled_file_system.h"

namespace chromeos {
namespace file_system_provider {

std::unique_ptr<ProvidedFileSystemInterface>
ExtensionProvider::CreateProvidedFileSystem(
    Profile* profile,
    const ProvidedFileSystemInfo& file_system_info) {
  DCHECK(profile);
  return base::MakeUnique<ThrottledFileSystem>(
      base::MakeUnique<ProvidedFileSystem>(profile, file_system_info));
}

Capabilities ExtensionProvider::GetCapabilities(Profile* profile,
                                                ProviderId& provider_id) {
  Service* service = Service::Get(profile);
  ProvidingExtensionInfo providing_extension_info;
  service->GetProvidingExtensionInfo(provider_id.GetExtensionId(),
                                     &providing_extension_info);

  return Capabilities(providing_extension_info.capabilities.configurable(),
                      providing_extension_info.capabilities.watchable(),
                      providing_extension_info.capabilities.multiple_mounts());
}

ExtensionProvider::ExtensionProvider() {}

}  // namespace file_system_provider
}  // namespace chromeos
