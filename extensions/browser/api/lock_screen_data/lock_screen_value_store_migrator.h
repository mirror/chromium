// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_LOCK_SCREEN_DATA_LOCK_SCREEN_VALUE_STORE_MIGRATOR_H_
#define EXTENSIONS_BROWSER_API_LOCK_SCREEN_DATA_LOCK_SCREEN_VALUE_STORE_MIGRATOR_H_

#include <set>
#include <string>

#include "base/callback.h"

namespace extensions {
namespace lock_screen_data {

class LockScreenValueStoreMigrator {
 public:
  virtual ~LockScreenValueStoreMigrator() {}

  using ExtensionMigratedCallback =
      base::Callback<void(const std::string& extension_id)>;
  virtual void Run(const std::set<std::string>& extensions_to_migrate,
                   const ExtensionMigratedCallback& callback) = 0;
  virtual void ClearDataForExtension(const std::string& extension_id,
                                     const base::Closure& callback) = 0;
  virtual bool MigratingExtensionData(
      const std::string& extension_id) const = 0;
};

}  // namespace lock_screen_data
}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_LOCK_SCREEN_DATA_LOCK_SCREEN_VALUE_STORE_MIGRATOR_H_
