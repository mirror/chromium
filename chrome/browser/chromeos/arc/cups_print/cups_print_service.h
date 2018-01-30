// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_CUPS_PRINT_ARC_CUPS_PRINT_SERVICE_H_
#define CHROME_BROWSER_CHROMEOS_ARC_CUPS_PRINT_ARC_CUPS_PRINT_SERVICE_H_

#include "base/files/file.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/threading/thread_checker.h"
#include "components/arc/common/cups_print.mojom.h"
#include "components/arc/connection_observer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace arc {

class ArcBridgeService;

class ArcCupsPrintService : public KeyedService,
                            public ConnectionObserver<mojom::CupsPrintInstance>,
                            public mojom::CupsPrintHost {
 public:
  // Returns singleton instance for the given BrowserContext,
  // or nullptr if the browser |context| is not allowed to use ARC.
  static ArcCupsPrintService* GetForBrowserContext(
      content::BrowserContext* context);

  ArcCupsPrintService(content::BrowserContext* context,
                      ArcBridgeService* bridge_service);
  ~ArcCupsPrintService() override;

  // ConnectionObserver<mojom::CupsPrintInstance> overrides;
  void OnConnectionReady() override;

  // mojom::CupsPrintHost override:
  void SendIppRequest(bool new_connection,
                      const std::vector<uint8_t>& headers,
                      const std::vector<uint8_t>& body,
                      SendIppRequestCallback callback) override;
  void Ask(const std::string& message, AskCallback cb) override;

 private:
  // Opens the pdf file at |file_path|.
  // If given |file_path| is nullopt, do nothing.
  void OpenPdf(base::Optional<base::FilePath> file_path) const;

  THREAD_CHECKER(thread_checker_);

  ArcBridgeService* const arc_bridge_service_;  // Owned by ArcServiceManager.

  base::WeakPtrFactory<ArcCupsPrintService> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(ArcCupsPrintService);
};

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_CUPS_PRINT_ARC_CUPS_PRINT_SERVICE_H_
