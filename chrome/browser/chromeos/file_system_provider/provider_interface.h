// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef CHROME_BROWSER_CHROMEOS_FILE_SYSTEM_PROVIDER_PROVIDER_INTERFACE_H_
#define CHROME_BROWSER_CHROMEOS_FILE_SYSTEM_PROVIDER_PROVIDER_INTERFACE_H_

#include <memory>
#include <string>

#include "chrome/browser/profiles/profile.h"

namespace chromeos {
namespace file_system_provider {

class ProvidedFileSystemInterface;
class ProvidedFileSystemInfo;
class ProviderId;

struct Capabilities {
  Capabilities(bool configurable, bool watchable, bool multiple_mounts)
      : configurable(configurable),
        watchable(watchable),
        multiple_mounts(multiple_mounts) {}

  bool configurable;
  bool watchable;
  bool multiple_mounts;
};

class ProviderInterface {
 public:
  ProviderInterface() {}
  virtual ~ProviderInterface() {}

  // Returns a pointer to a created file system.
  virtual std::unique_ptr<ProvidedFileSystemInterface> CreateProvidedFileSystem(
      Profile* profile,
      const ProvidedFileSystemInfo& file_system_info) = 0;

  // Returns the capabilites of file system with |provider_id|.
  virtual Capabilities GetCapabilities(Profile* profile,
                                       const ProviderId& provider_id) = 0;
};

}  // namespace file_system_provider
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_FILE_SYSTEM_PROVIDER_PROVIDER_INTERFACE_H_
