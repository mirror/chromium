// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/print/arc_print_service.h"

#include "base/memory/singleton.h"
#include "chrome/browser/printing/print_job.h"
#include "chrome/browser/printing/print_job_worker.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_browser_context_keyed_service_factory_base.h"
#include "printing/printed_pages_source.h"

namespace arc {
namespace {

// Singleton factory for ArcPrintService.
class ArcPrintServiceFactory
    : public internal::ArcBrowserContextKeyedServiceFactoryBase<
          ArcPrintService,
          ArcPrintServiceFactory> {
 public:
  // Factory name used by ArcBrowserContextKeyedServiceFactoryBase.
  static constexpr const char* kName = "ArcPrintServiceFactory";

  static ArcPrintServiceFactory* GetInstance() {
    return base::Singleton<ArcPrintServiceFactory>::get();
  }

 private:
  friend base::DefaultSingletonTraits<ArcPrintServiceFactory>;
  ArcPrintServiceFactory() = default;
  ~ArcPrintServiceFactory() override = default;
};

}  // namespace

// static
ArcPrintService* ArcPrintService::GetForBrowserContext(
    content::BrowserContext* context) {
  return ArcPrintServiceFactory::GetForBrowserContext(context);
}

ArcPrintService::ArcPrintService(content::BrowserContext* context,
                                 ArcBridgeService* bridge_service)
    : arc_bridge_service_(bridge_service),
      binding_(this),
      weak_ptr_factory_(this) {
  arc_bridge_service_->print()->AddObserver(this);
}

ArcPrintService::~ArcPrintService() {
  arc_bridge_service_->print()->RemoveObserver(this);
}

void ArcPrintService::OnInstanceReady() {
  mojom::PrintInstance* print_instance =
      ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->print(), Init);
  DCHECK(print_instance);
  mojom::PrintHostPtr host_proxy;
  binding_.Bind(mojo::MakeRequest(&host_proxy));
  print_instance->Init(std::move(host_proxy));
}

void ArcPrintService::PrintDeprecated(mojo::ScopedHandle pdf_data) {
  LOG(ERROR) << "ArcPrintService::Print(ScopedHandle) is deprecated.";
}

class ArcPagesSource : public printing::PrintedPagesSource {
 public:
  base::string16 RenderSourceName() override {
    // TODO
    return base::string16();
  }
};

class ArcPrintJobWorkerOwner : public printing::PrintJobWorkerOwner {
 public:
  ArcPrintJobWorkerOwner() : cookie_(printing::PrintSettings::NewCookie()) {}

  void GetSettingsDone(const printing::PrintSettings& new_settings,
                       printing::PrintingContext::Result result) override {
    NOTREACHED();
  }

  std::unique_ptr<printing::PrintJobWorker> DetachWorker(
      printing::PrintJobWorkerOwner* new_owner) override {
    // TODO
    return nullptr;
  }

  const printing::PrintSettings& settings() const override {
    // TODO
    return settings_;
  }

  int cookie() const override { return cookie_; }

  ~ArcPrintJobWorkerOwner() override {}

  printing::PrintSettings settings_;
  int cookie_;
};

void ArcPrintService::Print(mojom::ArcPrintJobPtr printJob) {
  LOG(ERROR) << "ArcPrintService::Print()";
  scoped_refptr<printing::PrintJob> job(new printing::PrintJob());
  ArcPagesSource source;
  ArcPrintJobWorkerOwner owner;
  job->Initialize(&owner, &source, 1);
  job->DisconnectSource();
  job->StartPrinting();
}

void ArcPrintService::Cancel(mojom::ArcPrintJobPtr printJob) {}

void ArcPrintService::StartPrinterDiscovery(
    const std::string& sessionId,
    const std::vector<std::string>& printerIds) {
  LOG(ERROR) << "ArcPrintService::StartPrinterDiscovery()";
  auto* instance =
      ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->print(), AddPrinters);
  auto mediaSize = mojom::PrintMediaSize::New("sizeId", "A4", 8270, 11700);
  auto resolution =
      mojom::PrintResolution::New("resolutionId", "360dpi", 360, 360);
  auto margins = mojom::PrintMargins::New(0, 0, 0, 0);
  auto defaults = mojom::ArcPrintAttributes::New(
      mediaSize->Clone(), resolution->Clone(), margins->Clone(),
      mojom::PrintColorMode::MONOCHROME, mojom::PrintDuplexMode::NONE);
  std::vector<mojom::PrintMediaSizePtr> mediaSizes;
  mediaSizes.push_back(std::move(mediaSize));
  std::vector<mojom::PrintResolutionPtr> resolutions;
  resolutions.push_back(std::move(resolution));
  auto capabilities = mojom::PrinterCapabilities::New(
      std::move(mediaSizes), std::move(resolutions), std::move(margins),
      mojom::PrintColorMode::MONOCHROME, mojom::PrintDuplexMode::NONE,
      std::move(defaults));
  std::vector<mojom::ArcPrinterInfoPtr> printers;
  printers.push_back(mojom::ArcPrinterInfo::New(
      "printerId", "Mock ARC Printer", mojom::PrinterStatus::IDLE,
      base::Optional<std::string>(), base::Optional<std::string>(),
      std::move(capabilities)));
  instance->AddPrinters(sessionId, std::move(printers));
}

void ArcPrintService::StopPrinterDiscovery(const std::string& sessionId) {}

void ArcPrintService::ValidatePrinters(
    const std::string& sessionId,
    const std::vector<std::string>& printerIds) {}

void ArcPrintService::StartPrinterStateTracking(const std::string& sessionId,
                                                const std::string& printerId) {}

void ArcPrintService::StopPrinterStateTracking(const std::string& sessionId,
                                               const std::string& printerId) {}

void ArcPrintService::DestroyDiscoverySession(const std::string& sessionId) {}

}  // namespace arc
