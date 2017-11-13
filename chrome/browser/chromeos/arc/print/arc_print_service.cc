// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/print/arc_print_service.h"

#include <cups/cups.h>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind_helpers.h"
#include "base/memory/singleton.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/printing/print_job.h"
#include "chrome/browser/printing/print_job_worker.h"
#include "chrome/browser/profiles/profile.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_browser_context_keyed_service_factory_base.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_source.h"
#include "content/public/common/child_process_host.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "printing/backend/print_backend.h"
#include "printing/backend/print_backend_consts.h"
#include "printing/pdf_metafile_skia.h"
#include "printing/print_job_constants.h"
#include "printing/printed_document.h"
#include "printing/units.h"

namespace arc {
namespace {

constexpr int kMilsPerInch = 1000;

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

// This creates a Metafile instance which is a wrapper around a byte buffer at
// this point.
std::unique_ptr<printing::PdfMetafileSkia> ReadFileOnBlockingThread(
    base::File file,
    size_t data_size) {
  // TODO(vkuzkokov) Can we make give pipe to CUPS directly?
  std::vector<char> buf(data_size);
  int bytes = file.ReadAtCurrentPos(buf.data(), data_size);
  if (bytes < 0)
    PLOG(ERROR) << "Error reading PDF";

  if (static_cast<size_t>(bytes) != data_size)
    return nullptr;

  file.Close();

  auto metafile = std::make_unique<printing::PdfMetafileSkia>(
      printing::SkiaDocumentType::PDF);
  if (!metafile->InitFromData(buf.data(), buf.size())) {
    LOG(ERROR) << "Failed to initialize PDF metafile";
    return nullptr;
  }
  return metafile;
}

// This represents a single request from container. Object of this class
// self-destructs when the request is completed, successfully or otherwise.
class ArcPrintRequest : public mojom::PrintJobHost,
                        public content::NotificationObserver {
 public:
  ArcPrintRequest(mojo::InterfaceRequest<mojom::PrintJobHost> request,
                  mojom::PrintJobInstancePtr instance,
                  std::unique_ptr<printing::PrintSettings> settings,
                  base::File file,
                  size_t data_size)
      : binding_(this, std::move(request)), instance_(std::move(instance)) {
    // We don't delete until we received both responses. Therefore, Unretained
    // is safe.
    base::PostTaskWithTraitsAndReplyWithResult(
        FROM_HERE, {base::MayBlock()},
        base::BindOnce(&ReadFileOnBlockingThread, std::move(file), data_size),
        base::BindOnce(&ArcPrintRequest::OnFileRead, base::Unretained(this)));
    content::BrowserThread::PostTask(
        content::BrowserThread::IO, FROM_HERE,
        base::BindOnce(&ArcPrintRequest::CreateQueryOnIOThread,
                       base::Unretained(this), std::move(settings)));
  }

  // mojom::PrintJobHost override:
  void Cancel() override {
    // TODO(vkuzkokov) implement.
  }

  // content::NotificationObserver override:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override {
    const printing::JobEventDetails& event_details =
        *content::Details<printing::JobEventDetails>(details).ptr();
    // TODO(vkuzkokov) consider updating container on other events.
    if (event_details.type() == printing::JobEventDetails::JOB_DONE ||
        event_details.type() == printing::JobEventDetails::FAILED) {
      instance_->Complete();
      delete this;
    }
  }

 private:
  // Store Metafile and start printing if PrintJob is created as well.
  void OnFileRead(std::unique_ptr<printing::PdfMetafileSkia> metafile) {
    metafile_ = std::move(metafile);
    StartPrintingIfReady();
  }

  // Creating PrinterQuery can only be done on IO thread.
  void CreateQueryOnIOThread(
      std::unique_ptr<printing::PrintSettings> settings) {
    auto query = base::MakeRefCounted<printing::PrinterQuery>(
        content::ChildProcessHost::kInvalidUniqueID,
        content::ChildProcessHost::kInvalidUniqueID);
    query->SetSettings(
        std::move(settings),
        base::BindOnce(&ArcPrintRequest::OnSetSettingsDoneOnIOThread,
                       base::Unretained(this), query));
  }

  // Send initialized PrinterQuery to UI thread.
  void OnSetSettingsDoneOnIOThread(
      scoped_refptr<printing::PrinterQuery> query) {
    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::BindOnce(&ArcPrintRequest::OnSetSettingsDone,
                       base::Unretained(this), query));
  }

  // Create PrintJob and start printing if Metafile is created as well.
  void OnSetSettingsDone(scoped_refptr<printing::PrinterQuery> query) {
    job_ = base::MakeRefCounted<printing::PrintJob>();
    job_->Initialize(query.get(), base::string16() /* name */,
                     1 /* page_count */);
    registrar_.Add(this, chrome::NOTIFICATION_PRINT_JOB_EVENT,
                   content::Source<printing::PrintJob>(job_.get()));
    StartPrintingIfReady();
  }

  // If both PrintJob and Metafile are available start printing.
  void StartPrintingIfReady() {
    if (!job_ || !metafile_)
      return;

    printing::PrintedDocument* document = job_->document();
    document->SetPage(0 /* page_number */, std::move(metafile_) /* metafile */,
                      gfx::Size() /* paper_size */,
                      gfx::Rect() /* page_rect */);
    job_->StartPrinting();
  }

  mojo::Binding<mojom::PrintJobHost> binding_;  // Binds |this|.
  mojom::PrintJobInstancePtr instance_;
  std::unique_ptr<printing::PdfMetafileSkia> metafile_;
  scoped_refptr<printing::PrintJob> job_;
  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(ArcPrintRequest);
};

std::unique_ptr<printing::PrinterSemanticCapsAndDefaults>
FetchCapabilitiesOnBlockingThread(const std::string& printer_id) {
  scoped_refptr<printing::PrintBackend> backend(
      printing::PrintBackend::CreateInstance(nullptr));
  auto caps = std::make_unique<printing::PrinterSemanticCapsAndDefaults>();
  if (!backend->GetPrinterSemanticCapsAndDefaults(printer_id, caps.get())) {
    LOG(ERROR) << "Failed to get caps for " << printer_id;
    return nullptr;
  }
  return caps;
}

// Transform printer info to Mojo type and add capabilities, if present.
mojom::PrinterInfoPtr ToArcPrinter(
    const chromeos::Printer& printer,
    std::unique_ptr<printing::PrinterSemanticCapsAndDefaults> caps) {
  return mojom::PrinterInfo::New(
      printer.id(), printer.display_name(), mojom::PrinterStatus::IDLE,
      printer.description(), base::nullopt,
      caps ? *caps
           : base::Optional<printing::PrinterSemanticCapsAndDefaults>());
}

// PrinterDiscoverySessionHost implementation.
class DiscoverySession : public mojom::PrinterDiscoverySessionHost,
                         public chromeos::CupsPrintersManager::Observer {
 public:
  DiscoverySession(mojom::PrinterDiscoverySessionInstancePtr instance,
                   Profile* profile)
      : instance_(std::move(instance)),
        printers_manager_(chromeos::CupsPrintersManager::Create(profile)),
        configurer_(chromeos::PrinterConfigurer::Create(profile)),
        weak_factory_(this) {
    printers_manager_->AddObserver(this);
  }

  void SetBinding(
      mojo::StrongBindingPtr<mojom::PrinterDiscoverySessionHost> binding) {
    binding_ = binding;
  }

  ~DiscoverySession() override { printers_manager_->RemoveObserver(this); }

  // mojom::PrinterDiscoverySessionHost overrides:
  void StartPrinterDiscovery(
      const std::vector<std::string>& printer_ids) override {
    std::vector<mojom::PrinterInfoPtr> arc_printers;
    for (int i = 0; i < chromeos::CupsPrintersManager::kNumPrinterClasses;
         i++) {
      std::vector<chromeos::Printer> printers = printers_manager_->GetPrinters(
          static_cast<chromeos::CupsPrintersManager::PrinterClass>(i));
      for (const auto& printer : printers)
        arc_printers.push_back(ToArcPrinter(printer, nullptr));
    }
    if (!arc_printers.empty())
      instance_->AddPrinters(std::move(arc_printers));
  }

  void StopPrinterDiscovery() override {
    // Do nothing
  }

  void ValidatePrinters(const std::vector<std::string>& printer_ids) override {
    // TODO(vkuzkokov) implement or determine that we don't need to.
  }

  void StartPrinterStateTracking(const std::string& printer_id) override {
    std::unique_ptr<chromeos::Printer> printer =
        printers_manager_->GetPrinter(printer_id);
    if (!printer) {
      RemovePrinter(printer_id);
      return;
    }
    if (printers_manager_->IsPrinterInstalled(*printer)) {
      PrinterInstalled(std::move(printer), chromeos::kSuccess);
      return;
    }
    const chromeos::Printer& printer_ref = *printer;
    configurer_->SetUpPrinter(
        printer_ref,
        base::BindOnce(&DiscoverySession::PrinterInstalled,
                       weak_factory_.GetWeakPtr(), std::move(printer)));
  }

  void StopPrinterStateTracking(const std::string& printer_id) override {
    // Do nothing
  }

  void DestroyDiscoverySession() override { binding_->Close(); }

  // chromeos::CupsPrintersManager::Observer override:
  void OnPrintersChanged(
      chromeos::CupsPrintersManager::PrinterClass printer_class,
      const std::vector<chromeos::Printer>& printers) override {
    // TODO(vkuzkokov) remove missing printers and only add new ones.
    std::vector<mojom::PrinterInfoPtr> arc_printers;
    for (const auto& printer : printers)
      arc_printers.push_back(ToArcPrinter(printer, nullptr));

    instance_->AddPrinters(std::move(arc_printers));
  }

 private:
  // Fetch capabilities for newly installed printer.
  void PrinterInstalled(std::unique_ptr<chromeos::Printer> printer,
                        chromeos::PrinterSetupResult result) {
    if (result != chromeos::kSuccess) {
      RemovePrinter(printer->id());
      return;
    }
    printers_manager_->PrinterInstalled(*printer);
    const std::string& printer_id = printer->id();
    base::PostTaskWithTraitsAndReplyWithResult(
        FROM_HERE, {base::MayBlock()},
        base::BindOnce(&FetchCapabilitiesOnBlockingThread, printer_id),
        base::BindOnce(&DiscoverySession::CapabilitiesReceived,
                       weak_factory_.GetWeakPtr(), std::move(printer)));
  }

  // Remove from the list of available printers.
  void RemovePrinter(const std::string& printer_id) {
    instance_->RemovePrinters(std::vector<std::string>{printer_id});
  }

  // Transform printer capabilities to mojo type and send to container.
  void CapabilitiesReceived(
      std::unique_ptr<chromeos::Printer> printer,
      std::unique_ptr<printing::PrinterSemanticCapsAndDefaults> caps) {
    if (!caps) {
      RemovePrinter(printer->id());
      return;
    }
    std::vector<mojom::PrinterInfoPtr> arc_printers;
    arc_printers.push_back(ToArcPrinter(*printer, std::move(caps)));
    instance_->AddPrinters(std::move(arc_printers));
  }

  mojom::PrinterDiscoverySessionInstancePtr instance_;
  std::unique_ptr<chromeos::CupsPrintersManager> printers_manager_;
  std::unique_ptr<chromeos::PrinterConfigurer> configurer_;

  // Binds |this|.
  mojo::StrongBindingPtr<mojom::PrinterDiscoverySessionHost> binding_;

  base::WeakPtrFactory<DiscoverySession> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(DiscoverySession);
};

// Get requested color mode from Mojo type.
// |mode| is a bitfield but must have exactly one mode set here.
printing::ColorModel FromArcColorMode(mojom::PrintColorMode mode) {
  switch (mode) {
    case mojom::PrintColorMode::MONOCHROME:
      return printing::GRAY;
    case mojom::PrintColorMode::COLOR:
      return printing::COLOR;
  }
  NOTREACHED();
}

// Get requested duplex mode from Mojo type.
// |mode| is a bitfield but must have exactly one mode set here.
printing::DuplexMode FromArcDuplexMode(mojom::PrintDuplexMode mode) {
  switch (mode) {
    case mojom::PrintDuplexMode::NONE:
      return printing::SIMPLEX;
    case mojom::PrintDuplexMode::LONG_EDGE:
      return printing::LONG_EDGE;
    case mojom::PrintDuplexMode::SHORT_EDGE:
      return printing::SHORT_EDGE;
  }
  NOTREACHED();
}

}  // namespace

// static
ArcPrintService* ArcPrintService::GetForBrowserContext(
    content::BrowserContext* context) {
  return ArcPrintServiceFactory::GetForBrowserContext(context);
}

ArcPrintService::ArcPrintService(content::BrowserContext* context,
                                 ArcBridgeService* bridge_service)
    : profile_(Profile::FromBrowserContext(context)),
      arc_bridge_service_(bridge_service),
      binding_(this) {
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

void ArcPrintService::Print(mojom::PrintJobInstancePtr instance,
                            mojom::PrintJobRequestPtr print_job,
                            PrintCallback callback) {
  instance->Start();

  const mojom::PrintAttributesPtr& attr = print_job->attributes;
  const mojom::PrintMediaSizePtr& arc_media = attr->media_size;
  const base::Optional<gfx::Size> resolution = attr->resolution;
  if (!arc_media || !resolution) {
    // TODO(vkuzkokov): localize
    instance->Fail(base::Optional<std::string>(
        base::in_place,
        "Print request must contain media size and resolution"));
    return;
  }
  const mojom::PrintMarginsPtr& margins = attr->min_margins;
  auto settings = std::make_unique<printing::PrintSettings>();

  gfx::Size size_mils(arc_media->width_mils, arc_media->height_mils);
  printing::PrintSettings::RequestedMedia media;
  media.size_microns =
      gfx::ScaleToRoundedSize(size_mils, printing::kMicronsPerMil);
  settings->set_requested_media(media);

  // TODO(vkuzkokov) Is it just max(dpm_hor, dpm_ver) as per
  // print_settings_conversion?
  float x_scale = static_cast<float>(resolution->width()) / kMilsPerInch;
  float y_scale = static_cast<float>(resolution->height()) / kMilsPerInch;
  settings->set_dpi_xy(resolution->width(), resolution->height());

  gfx::Rect area_mils(size_mils);
  if (margins) {
    area_mils.Inset(margins->left_mils, margins->top_mils, margins->right_mils,
                    margins->bottom_mils);
  }
  settings->SetPrinterPrintableArea(
      gfx::ScaleToRoundedSize(size_mils, x_scale, y_scale),
      gfx::ScaleToRoundedRect(area_mils, x_scale, y_scale), false);
  if (print_job->printer_id)
    settings->set_device_name(base::UTF8ToUTF16(print_job->printer_id.value()));

  // Android is weird.
  const printing::PageRanges& pages = print_job->pages;
  if (!pages.empty() && pages.back().to != std::numeric_limits<int>::max())
    settings->set_ranges(pages);

  settings->set_title(base::UTF8ToUTF16(print_job->document_name));
  settings->set_color(FromArcColorMode(attr->color_mode));
  settings->set_copies(print_job->copies);
  settings->set_duplex_mode(FromArcDuplexMode(attr->duplex_mode));
  mojo::edk::ScopedPlatformHandle scoped_handle;
  PassWrappedPlatformHandle(print_job->data.release().value(), &scoped_handle);

  mojom::PrintJobHostPtr host_proxy;

  // ArcPrintRequest manages its own lifetime.
  new ArcPrintRequest(
      mojo::MakeRequest(&host_proxy), std::move(instance), std::move(settings),
      base::File(scoped_handle.release().handle), print_job->data_size);
  std::move(callback).Run(std::move(host_proxy));
}

void ArcPrintService::CreateDiscoverySession(
    mojom::PrinterDiscoverySessionInstancePtr instance,
    CreateDiscoverySessionCallback callback) {
  mojom::PrinterDiscoverySessionHostPtr host_proxy;
  mojo::InterfaceRequest<mojom::PrinterDiscoverySessionHost> request =
      mojo::MakeRequest(&host_proxy);
  auto session =
      std::make_unique<DiscoverySession>(std::move(instance), profile_);
  DiscoverySession* session_ptr = session.get();
  session_ptr->SetBinding(
      mojo::MakeStrongBinding(std::move(session), std::move(request)));
  std::move(callback).Run(std::move(host_proxy));
}

}  // namespace arc
