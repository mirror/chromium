// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_PRINT_ARC_PRINT_SERVICE_H_
#define CHROME_BROWSER_CHROMEOS_ARC_PRINT_ARC_PRINT_SERVICE_H_

#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/printing/cups_printers_manager.h"
#include "chrome/browser/chromeos/printing/printer_configurer.h"
#include "components/arc/common/print.mojom.h"
#include "components/arc/instance_holder.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace printing {
struct PrinterSemanticCapsAndDefaults;
}  // namespace printing

namespace arc {

class ArcBridgeService;

class ArcPrintService : public KeyedService,
                        public InstanceHolder<mojom::PrintInstance>::Observer,
                        public mojom::PrintHost,
                        public chromeos::CupsPrintersManager::Observer {
 public:
  // Returns singleton instance for the given BrowserContext,
  // or nullptr if the browser |context| is not allowed to use ARC.
  static ArcPrintService* GetForBrowserContext(
      content::BrowserContext* context);

  ArcPrintService(content::BrowserContext* context,
                  ArcBridgeService* bridge_service);
  ~ArcPrintService() override;

  // InstanceHolder<mojom::PrintInstance>::Observer override:
  void OnInstanceReady() override;

  // mojom::PrintHost overrides:
  void PrintDeprecated(mojo::ScopedHandle pdf_data) override;

  void Print(mojom::ArcPrintJobPtr print_job) override;

  void Cancel(mojom::ArcPrintJobPtr print_job) override;

  void StartPrinterDiscovery(
      const std::string& session_id,
      const std::vector<std::string>& printer_ids) override;

  void StopPrinterDiscovery(const std::string& session_id) override;

  void ValidatePrinters(const std::string& session_id,
                        const std::vector<std::string>& printer_ids) override;

  void StartPrinterStateTracking(const std::string& session_id,
                                 const std::string& printer_id) override;

  void StopPrinterStateTracking(const std::string& session_id,
                                const std::string& printer_id) override;

  void DestroyDiscoverySession(const std::string& session_id) override;

  void OnPrintersChanged(
      chromeos::CupsPrintersManager::PrinterClass printer_class,
      const std::vector<chromeos::Printer>& printers) override;

 private:
  void RemovePrinter(const std::string& session_id,
                     const std::string& printer_id);
  void PrinterInstalled(const std::string& session_id,
                        std::unique_ptr<chromeos::Printer> printer,
                        chromeos::PrinterSetupResult result);
  void CapabilitiesReceived(
      const std::string& session_id,
      std::unique_ptr<chromeos::Printer> printer,
      std::unique_ptr<printing::PrinterSemanticCapsAndDefaults> capabilities);

  void PrintingDone(const std::vector<int8_t>& job_id);

  ArcBridgeService* const arc_bridge_service_;  // Owned by ArcServiceManager.
  mojo::Binding<mojom::PrintHost> binding_;
  std::unique_ptr<chromeos::CupsPrintersManager> printers_manager_;
  std::unique_ptr<chromeos::PrinterConfigurer> configurer_;
  std::set<std::string> discovering_sessions_;

  base::WeakPtrFactory<ArcPrintService> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(ArcPrintService);
};

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_PRINT_ARC_PRINT_SERVICE_H_
