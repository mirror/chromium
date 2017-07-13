// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_PRINTING_PRINTERS_SYNC_MANAGER_FACTORY_H_
#define CHROME_BROWSER_CHROMEOS_PRINTING_PRINTERS_SYNC_MANAGER_FACTORY_H_

#include "base/lazy_instance.h"
#include "base/macros.h"
#include "chrome/browser/chromeos/printing/printers_sync_manager.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace content {
class BrowserContext;
}

namespace chromeos {

class PrintersSyncManagerFactory : public BrowserContextKeyedServiceFactory {
 public:
  static PrintersSyncManager* GetForBrowserContext(
      content::BrowserContext* context);

  static PrintersSyncManagerFactory* GetInstance();

 protected:
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;

 private:
  friend struct base::LazyInstanceTraitsBase<PrintersSyncManagerFactory>;

  PrintersSyncManagerFactory();
  ~PrintersSyncManagerFactory() override;

  // BrowserContextKeyedServiceFactory implementation:
  PrintersSyncManager* BuildServiceInstanceFor(
      content::BrowserContext* browser_context) const override;

  DISALLOW_COPY_AND_ASSIGN(PrintersSyncManagerFactory);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_PRINTING_PRINTERS_SYNC_MANAGER_FACTORY_H_
