// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_PRINT_ARC_PRINT_SERVICE_H_
#define CHROME_BROWSER_CHROMEOS_ARC_PRINT_ARC_PRINT_SERVICE_H_

#include <string>
#include <unordered_map>

#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/printing/cups_print_job_manager.h"
#include "components/arc/common/print.mojom.h"
#include "components/arc/connection_observer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/binding.h"

class Profile;

namespace content {
class BrowserContext;
}  // namespace content

namespace chromeos {
class CupsPrintJob;
}  // namespace chromeos

namespace arc {

class ArcBridgeService;

class ArcPrintService : public KeyedService,
                        public ConnectionObserver<mojom::PrintInstance>,
                        public chromeos::CupsPrintJobManager::Observer,
                        public mojom::PrintHost {
 public:
  // Returns singleton instance for the given BrowserContext,
  // or nullptr if the browser |context| is not allowed to use ARC.
  static ArcPrintService* GetForBrowserContext(
      content::BrowserContext* context);

  ArcPrintService(content::BrowserContext* context,
                  ArcBridgeService* bridge_service);
  ~ArcPrintService() override;

  // ConnectionObserver<mojom::PrintInstance> override:
  void OnConnectionReady() override;

  // mojom::PrintHost overrides:
  void PrintDeprecated(mojo::ScopedHandle pdf_data) override;
  void Print(mojom::PrintJobInstancePtr instance,
             mojom::PrintJobRequestPtr print_job,
             PrintCallback callback) override;
  void CreateDiscoverySession(
      mojom::PrinterDiscoverySessionInstancePtr instance,
      CreateDiscoverySessionCallback callback) override;

 protected:
  // chromeos::CupsPrintJobManager::Observer overrides:
  void OnPrintJobCreated(chromeos::CupsPrintJob* job) override;
  void OnPrintJobCancelled(chromeos::CupsPrintJob* job) override;
  void OnPrintJobError(chromeos::CupsPrintJob* job) override;
  void OnPrintJobDone(chromeos::CupsPrintJob* job) override;

 private:
  class PrintJobHostImpl;

  void OnConnectionError();

  Profile* const profile_;                      // Owned by ProfileManager.
  ArcBridgeService* const arc_bridge_service_;  // Owned by ArcServiceManager.
  mojo::Binding<mojom::PrintHost> binding_;

  // Managed by PrintJobHostImpl instances.
  std::unordered_map<std::string, PrintJobHostImpl*> jobs_;

  DISALLOW_COPY_AND_ASSIGN(ArcPrintService);
};

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_PRINT_ARC_PRINT_SERVICE_H_
