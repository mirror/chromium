// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/samba_client/create.h"

#include <stddef.h>

#include "base/memory/ptr_util.h"
#include "chrome/browser/chromeos/file_system_provider/provided_file_system.h"
#include "chrome/browser/chromeos/file_system_provider/provided_file_system_info.h"
#include "chrome/browser/chromeos/file_system_provider/provided_file_system_interface.h"
#include "chrome/browser/chromeos/file_system_provider/throttled_file_system.h"

namespace chromeos {
namespace samba_client {

using file_system_provider::ProvidedFileSystemInterface;

std::unique_ptr<ProvidedFileSystemInterface> CreateSambaProvidedFileSystem(
    const file_system_provider::ProvidedFileSystemInfo& file_system_info) {
  return base::MakeUnique<file_system_provider::ThrottledFileSystem>(
      base::MakeUnique<SmbProvidedFileSystem>(file_system_info));
}

}  // namespace samba_client
}  // namespace chromeos
