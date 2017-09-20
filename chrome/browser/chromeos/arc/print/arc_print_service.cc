// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/print/arc_print_service.h"

#include "base/memory/singleton.h"
#include "chrome/browser/printing/print_job.h"
#include "chrome/browser/printing/print_job_worker.h"
#include "chrome/browser/profiles/profile.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_browser_context_keyed_service_factory_base.h"
#include "content/public/common/child_process_host.h"
#include "printing/backend/print_backend.h"
#include "printing/print_job_constants.h"
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

class ArcPagesSource : public printing::PrintedPagesSource {
 public:
  base::string16 RenderSourceName() override {
    // TODO
    return base::string16();
  }
};

void SetSettingsDoneOnUI(scoped_refptr<printing::PrintJobWorkerOwner> query) {
  ArcPagesSource source;
  scoped_refptr<printing::PrintJob> job(new printing::PrintJob());
  job->Initialize(query.get(), &source, 1);
  job->DisconnectSource();
  job->StartPrinting();
}

void SetSettingsDone(scoped_refptr<printing::PrintJobWorkerOwner> query) {
  content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
                                   base::BindOnce(&SetSettingsDoneOnUI, query));
}

void PrintOnIO() {
  scoped_refptr<printing::PrinterQuery> query(
      new printing::PrinterQuery(content::ChildProcessHost::kInvalidUniqueID,
                                 content::ChildProcessHost::kInvalidUniqueID));
  std::unique_ptr<base::DictionaryValue> settings(new base::DictionaryValue());
  // TODO actual settings
  settings->SetBoolean(printing::kSettingHeaderFooterEnabled, false);
  settings->SetBoolean(printing::kSettingShouldPrintBackgrounds, false);
  settings->SetBoolean(printing::kSettingShouldPrintSelectionOnly, false);
  settings->SetBoolean(printing::kSettingCollate, true);
  settings->SetInteger(printing::kSettingCopies, 1);
  settings->SetInteger(printing::kSettingColor, printing::COLOR);
  settings->SetInteger(printing::kSettingDuplexMode, printing::SIMPLEX);
  settings->SetBoolean(printing::kSettingLandscape, false);
  settings->SetString(printing::kSettingDeviceName, "");
  settings->SetInteger(printing::kSettingScaleFactor, 100);
  settings->SetBoolean(printing::kSettingRasterizePdf, false);
  settings->SetBoolean(printing::kSettingPrintToPDF, false);
  settings->SetBoolean(printing::kSettingCloudPrintDialog, false);
  settings->SetBoolean(printing::kSettingPrintWithPrivet, false);
  settings->SetBoolean(printing::kSettingPrintWithExtension, false);
  query->SetSettings(std::move(settings), base::Bind(&SetSettingsDone, query));
}

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
      printers_manager_(chromeos::CupsPrintersManager::Create(
          Profile::FromBrowserContext(context))),
      weak_ptr_factory_(this) {
  arc_bridge_service_->print()->AddObserver(this);
  printers_manager_->AddObserver(this);
}

ArcPrintService::~ArcPrintService() {
  arc_bridge_service_->print()->RemoveObserver(this);
}

void ArcPrintService::OnInstanceReady() {
  mojom::PrintInstance* print_instance =
      ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->print(), Init);
  DCHECK(print_instance);
  mojom::PrintHostPtr host_proxy;
  binding_.Close();
  binding_.Bind(mojo::MakeRequest(&host_proxy));
  print_instance->Init(std::move(host_proxy));
}

void ArcPrintService::PrintDeprecated(mojo::ScopedHandle pdf_data) {
  LOG(ERROR) << "ArcPrintService::Print(ScopedHandle) is deprecated.";
}

void ArcPrintService::Print(mojom::ArcPrintJobPtr print_job) {
  content::BrowserThread::PostTask(content::BrowserThread::IO, FROM_HERE,
                                   base::BindOnce(&PrintOnIO));
}

void ArcPrintService::Cancel(mojom::ArcPrintJobPtr print_job) {}

void ArcPrintService::StartPrinterDiscovery(
    const std::string& session_id,
    const std::vector<std::string>& printer_ids) {
  discovering_sessions_.insert(session_id);
  auto* instance =
      ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->print(), AddPrinters);
  std::vector<mojom::ArcPrinterInfoPtr> arc_printers;
  for (int i = 0; i < chromeos::CupsPrintersManager::kNumPrinterClasses; i++) {
    std::vector<chromeos::Printer> printers = printers_manager_->GetPrinters(
        (chromeos::CupsPrintersManager::PrinterClass)i);
    for (const auto& printer : printers) {
      arc_printers.push_back(mojom::ArcPrinterInfo::New(
          printer.id(), printer.display_name(), mojom::PrinterStatus::IDLE,
          base::Optional<std::string>(printer.description()), base::nullopt,
          nullptr));
    }
  }
  if (!arc_printers.empty()) {
    instance->AddPrinters(session_id, std::move(arc_printers));
  }
}

void ArcPrintService::StopPrinterDiscovery(const std::string& session_id) {
  discovering_sessions_.insert(session_id);
}

void ArcPrintService::ValidatePrinters(
    const std::string& session_id,
    const std::vector<std::string>& printer_ids) {}

void ArcPrintService::StartPrinterStateTracking(const std::string& session_id,
                                                const std::string& printer_id) {
  LOG(ERROR) << "StartPrinterStateTracking";
}

void ArcPrintService::StopPrinterStateTracking(const std::string& session_id,
                                               const std::string& printer_id) {}

void ArcPrintService::DestroyDiscoverySession(const std::string& session_id) {
  discovering_sessions_.insert(session_id);
}

void ArcPrintService::OnPrintersChanged(
    chromeos::CupsPrintersManager::PrinterClass printer_class,
    const std::vector<chromeos::Printer>& printers) {
  // TODO remove missing printers and only add new ones.
  auto* instance =
      ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->print(), AddPrinters);
  std::vector<mojom::ArcPrinterInfoPtr> arc_printers;
  for (const auto& printer : printers) {
    arc_printers.push_back(mojom::ArcPrinterInfo::New(
        printer.id(), printer.display_name(), mojom::PrinterStatus::IDLE,
        base::Optional<std::string>(printer.description()), base::nullopt,
        nullptr));
  }
  for (auto it = discovering_sessions_.begin();
       it != discovering_sessions_.end(); ++it) {
    if (it == discovering_sessions_.rbegin().base()) {
      instance->AddPrinters(*it, std::move(arc_printers));
    } else {
      std::vector<mojom::ArcPrinterInfoPtr> arc_printers_copy;
      for (const auto& arc_printer : arc_printers) {
        arc_printers_copy.push_back(arc_printer->Clone());
      }
      instance->AddPrinters(*it, std::move(arc_printers_copy));
    }
  }
}

}  // namespace arc
