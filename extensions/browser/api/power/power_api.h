// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_POWER_POWER_API_H_
#define EXTENSIONS_BROWSER_API_POWER_POWER_API_H_

#include <map>
#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "device/wake_lock/public/interfaces/wake_lock.mojom.h"
#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "extensions/browser/extension_function.h"
#include "extensions/browser/extension_registry_observer.h"
#include "extensions/common/api/power.h"

namespace content {
class BrowserContext;
}

namespace extensions {

// Implementation of the chrome.power.requestKeepAwake API.
class PowerRequestKeepAwakeFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("power.requestKeepAwake", POWER_REQUESTKEEPAWAKE)

 protected:
  ~PowerRequestKeepAwakeFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

// Implementation of the chrome.power.releaseKeepAwake API.
class PowerReleaseKeepAwakeFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("power.releaseKeepAwake", POWER_RELEASEKEEPAWAKE)

 protected:
  ~PowerReleaseKeepAwakeFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

// Handles calls made via the chrome.power API. There is a separate instance of
// this class for each profile, as requests are tracked by extension ID, but a
// regular and incognito profile will share the same instance.
class PowerAPI : public BrowserContextKeyedAPI,
                 public extensions::ExtensionRegistryObserver {
 public:
  typedef base::Callback<void(device::mojom::WakeLockType)>
      RequestWakeLockFunction;
  typedef base::Callback<void()> CancelWakeLockFunction;

  static PowerAPI* Get(content::BrowserContext* context);

  // BrowserContextKeyedAPI implementation.
  static BrowserContextKeyedAPIFactory<PowerAPI>* GetFactoryInstance();

  // Map from extension ID to the corresponding level for each extension
  // that has an outstanding request.
  using ExtensionLevelMap = std::map<std::string, api::power::Level>;
  const ExtensionLevelMap& extension_levels() const {
    return extension_levels_;
  }

  // Adds an extension lock at |level| for |extension_id|, replacing the
  // extension's existing lock, if any.
  void AddRequest(const std::string& extension_id, api::power::Level level);

  // Removes an extension lock for an extension. Calling this for an
  // extension id without a lock will do nothing.
  void RemoveRequest(const std::string& extension_id);

  // Replaces the function that will be called to request and cancel wake lock.
  // Passing an empty callback will revert to the default.
  void SetWakeLockFunctionForTesting(
      const RequestWakeLockFunction& request_function,
      const CancelWakeLockFunction& cancel_function);

  // Overridden from extensions::ExtensionRegistryObserver.
  void OnExtensionUnloaded(content::BrowserContext* browser_context,
                           const Extension* extension,
                           UnloadedExtensionReason reason) override;

 private:
  friend class BrowserContextKeyedAPIFactory<PowerAPI>;

  explicit PowerAPI(content::BrowserContext* context);
  ~PowerAPI() override;

  // Updates wake lock status and |current_level_| after iterating
  // over |extension_levels_|.
  void UpdateWakeLock();

  // BrowserContextKeyedAPI implementation.
  static const char* service_name() { return "PowerAPI"; }
  static const bool kServiceRedirectedInIncognito = true;
  static const bool kServiceIsCreatedWithBrowserContext = false;
  void Shutdown() override;

  content::BrowserContext* browser_context_;

  device::mojom::WakeLock* GetWakeLock();
  void RequestWakeLock(device::mojom::WakeLockType type);
  void CancelWakeLock();

  // Function that should be called to request and cancel the wake lock.
  // Tests can change this to record what would've been done instead of
  // actually changing the system power-saving settings.
  RequestWakeLockFunction request_wake_lock_function_;
  CancelWakeLockFunction cancel_wake_lock_function_;

  device::mojom::WakeLockPtr wake_lock_;
  bool has_wake_lock_;

  // Current level used by |wake_lock_|.
  api::power::Level current_level_;

  // Outstanding requests.
  ExtensionLevelMap extension_levels_;

  DISALLOW_COPY_AND_ASSIGN(PowerAPI);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_POWER_POWER_API_H_
