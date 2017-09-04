// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_PRINT_ARC_PRINT_SERVICE_H_
#define CHROME_BROWSER_CHROMEOS_ARC_PRINT_ARC_PRINT_SERVICE_H_

#include "base/files/file.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/threading/thread_checker.h"
#include "components/arc/common/print.mojom.h"
#include "components/arc/instance_holder.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace arc {

class ArcBridgeService;

class ArcPrintService : public KeyedService,
                        public InstanceHolder<mojom::PrintInstance>::Observer,
                        public mojom::PrintHost {
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

  void Print(mojom::ArcPrintJobPtr printJob) override;

  void Cancel(mojom::ArcPrintJobPtr printJob) override;

  void StartPrinterDiscovery(
      const std::string& sessionId,
      const std::vector<std::string>& printerIds) override;

  void StopPrinterDiscovery(const std::string& sessionId) override;

  void ValidatePrinters(const std::string& sessionId,
                        const std::vector<std::string>& printerIds) override;

  void StartPrinterStateTracking(const std::string& sessionId,
                                 const std::string& printerId) override;

  void StopPrinterStateTracking(const std::string& sessionId,
                                const std::string& printerId) override;

  void DestroyDiscoverySession(const std::string& sessionId) override;

 private:
  THREAD_CHECKER(thread_checker_);

  ArcBridgeService* const arc_bridge_service_;  // Owned by ArcServiceManager.
  mojo::Binding<mojom::PrintHost> binding_;

  base::WeakPtrFactory<ArcPrintService> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(ArcPrintService);
};

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_PRINT_ARC_PRINT_SERVICE_H_
