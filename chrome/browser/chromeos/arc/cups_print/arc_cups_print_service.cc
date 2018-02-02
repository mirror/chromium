// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/cups_print/arc_cups_print_service.h"

#include "base/memory/singleton.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_browser_context_keyed_service_factory_base.h"

namespace arc {
namespace {

// Singleton factory for ArcCupsPrintService.
class ArcCupsPrintServiceFactory
    : public internal::ArcBrowserContextKeyedServiceFactoryBase<
          ArcCupsPrintService,
          ArcCupsPrintServiceFactory> {
 public:
  // Factory name used by ArcBrowserContextKeyedServiceFactoryBase.
  static constexpr const char* kName = "ArcCupsPrintServiceFactory";

  static ArcCupsPrintServiceFactory* GetInstance() {
    return base::Singleton<ArcCupsPrintServiceFactory>::get();
  }

 private:
  friend base::DefaultSingletonTraits<ArcCupsPrintServiceFactory>;
  ArcCupsPrintServiceFactory() = default;
  ~ArcCupsPrintServiceFactory() override = default;
};

}  // namespace

// static
ArcCupsPrintService* ArcCupsPrintService::GetForBrowserContext(
    content::BrowserContext* context) {
  return ArcCupsPrintServiceFactory::GetForBrowserContext(context);
}

ArcCupsPrintService::ArcCupsPrintService(content::BrowserContext* context,
                                         ArcBridgeService* bridge_service)
    : arc_bridge_service_(bridge_service) {
  arc_bridge_service_->cups_print()->AddObserver(this);
  arc_bridge_service_->cups_print()->SetHost(this);
}

ArcCupsPrintService::~ArcCupsPrintService() {
  arc_bridge_service_->cups_print()->RemoveObserver(this);
  arc_bridge_service_->cups_print()->SetHost(nullptr);
}

void ArcCupsPrintService::OnConnectionReady() {}

}  // namespace arc
