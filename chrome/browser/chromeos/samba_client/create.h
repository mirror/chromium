// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_SAMBA_CLIENT_CREATE_H_
#define CHROME_BROWSER_CHROMEOS_SAMBA_CLIENT_CREATE_H_

#include "chrome/browser/chromeos/samba_client/smb_provided_file_system.h"

namespace chromeos {
namespace samba_client {

using file_system_provider::ProvidedFileSystemInterface;
using file_system_provider::ProvidedFileSystemInfo;

// Creates a Samba Provided Filesystem
std::unique_ptr<ProvidedFileSystemInterface> CreateSambaProvidedFileSystem(
    const ProvidedFileSystemInfo& file_system_info);

}  // namespace samba_client
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_SAMBA_CLIENT_CREATE_H_
