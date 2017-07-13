// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_RULES_MONITOR_SERVICE_H_
#define EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_RULES_MONITOR_SERVICE_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/scoped_observer.h"
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

// Observes loading and unloading of extensions to load and unload their
// rulesets for the Declarative Net Request API. Lives on the UI thread. Note: A
// separate instance of RulesMonitorService is not created for incognito.
class RulesMonitorService : public BrowserContextKeyedAPI,
                            public ExtensionRegistryObserver {
 public:
  // BrowserContextKeyedAPI implementation.
  static BrowserContextKeyedAPIFactory<RulesMonitorService>*
  GetFactoryInstance();

 private:
  friend class BrowserContextKeyedAPIFactory<RulesMonitorService>;

  // COMMENT: Other instances of BrowserContextKeyedAPI don't have private
  // constructors/destructors even though they friend their factory classes.

  // The constructor is kept private since this should only be created by the
  // BrowserContextKeyedAPIFactory.
  explicit RulesMonitorService(content::BrowserContext* browser_context);
  // The destructor is kept private since this should only be deleted
  // polymorphically.
  ~RulesMonitorService() override;

  // BrowserContextKeyedAPI implementation.
  static const char* service_name() { return "RulesMonitorService"; }

  // ExtensionRegistryObserver overrides:
  void OnExtensionLoaded(content::BrowserContext* browser_context,
                         const Extension* extension) override;
  void OnExtensionUnloaded(content::BrowserContext* browser_context,
                           const Extension* extension,
                           UnloadedExtensionReason reason) override;

  ScopedObserver<ExtensionRegistry, ExtensionRegistryObserver>
      registry_observer_;
  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(RulesMonitorService);
};

}  // namespace declarative_net_request
}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_RULES_MONITOR_SERVICE_H_
