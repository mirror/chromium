// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_RULES_MONITOR_SERVICE_H_
#define EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_RULES_MONITOR_SERVICE_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/scoped_observer.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "extensions/browser/extension_registry_observer.h"

namespace base {
class SequencedTaskRunner;
}

namespace content {
class BrowserContext;
}

namespace extensions {

namespace declarative_net_request {

// Handles loading and unloading of extension rulesets as the extensions are
// loaded and unloaded respectively.
// TODO incognito behavior, and see incognito flags on BrowserContextKeyedAPI.
class RulesMonitorService : public BrowserContextKeyedAPI,
                            ExtensionRegistryObserver {
 public:
  explicit RulesMonitorService(content::BrowserContext* browser_context);
  ~RulesMonitorService() override;

  // BrowserContextKeyedAPI implementation.
  static BrowserContextKeyedAPIFactory<RulesMonitorService>*
  GetFactoryInstance();

 private:
  friend class BrowserContextKeyedAPIFactory<RulesMonitorService>;

  // BrowserContextKeyedAPI implementation.
  static const char* service_name() { return "RulesMonitorService"; }

  // ExtensionRegistryObserver overrides:
  void OnExtensionLoaded(content::BrowserContext* browser_context,
                         const Extension* extension) override;
  void OnExtensionUnloaded(content::BrowserContext* browser_context,
                           const Extension* extension,
                           UnloadedExtensionReason reason) override;

  content::BrowserContext* browser_context_;
  ScopedObserver<ExtensionRegistry, ExtensionRegistryObserver>
      registry_observer_;
  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(RulesMonitorService);
};

}  // namespace declarative_net_request
}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_RULES_MONITOR_SERVICE_H_
