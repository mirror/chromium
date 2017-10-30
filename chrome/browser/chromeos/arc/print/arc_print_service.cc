// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/print/arc_print_service.h"

#include <cups/cups.h>
#include <sstream>

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

namespace arc {
namespace {

constexpr float kMicronsPerMil = 25.4f;
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
  // TODO Can we make give pipe to CUPS directly?
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
// self-destruct when the request is completed, successfully or otherwise.
class ArcPrintRequest : public content::NotificationObserver {
 public:
  ArcPrintRequest(std::unique_ptr<printing::PrintSettings> settings,
                  base::File file,
                  size_t data_size,
                  base::OnceClosure done_callback)
      : done_callback_(std::move(done_callback)) {
    // We don't delete until we received both responses. Therefore, Unretained
    // is safe.
    base::PostTaskWithTraitsAndReplyWithResult(
        FROM_HERE, {base::MayBlock()},
        base::BindOnce(&ReadFileOnBlockingThread, std::move(file), data_size),
        base::BindOnce(&ArcPrintRequest::OnFileRead, base::Unretained(this)));
    content::BrowserThread::PostTask(
        content::BrowserThread::IO, FROM_HERE,
        base::BindOnce(&ArcPrintRequest::SetSettingsOnIOThread,
                       base::Unretained(this), std::move(settings)));
  }

  // content::NotificationObserver override:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override {
    const printing::JobEventDetails& event_details =
        *content::Details<printing::JobEventDetails>(details).ptr();
    // TODO other events
    if (event_details.type() == printing::JobEventDetails::JOB_DONE) {
      std::move(done_callback_).Run();
      delete this;
    }
  }

 private:
  // Store Metafile and start printing if PrintJob is created as well.
  void OnFileRead(std::unique_ptr<printing::PdfMetafileSkia> metafile) {
    metafile_ = std::move(metafile);
    StartPrintingIfReady();
  }

  // Create PrinterQuery which can only be done on IO thread.
  void SetSettingsOnIOThread(
      std::unique_ptr<printing::PrintSettings> settings) {
    auto query = base::MakeRefCounted<printing::PrinterQuery>(
        content::ChildProcessHost::kInvalidUniqueID,
        content::ChildProcessHost::kInvalidUniqueID);
    query->SetSettings(
        std::move(settings),
        base::BindOnce(&ArcPrintRequest::OnSetSettingsDoneOnIOThread,
                       base::Unretained(this), query));
  }

  // Send newly created PrinterQuery to UI thread.
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

  // Wrapper around printing data.
  std::unique_ptr<printing::PdfMetafileSkia> metafile_;

  // Printing job for current request.
  scoped_refptr<printing::PrintJob> job_;

  // Registrar to subscribe to printing events.
  content::NotificationRegistrar registrar_;

  // Callback for printing being finished.
  base::OnceClosure done_callback_;
};

// Fetch capabilities on blocking thread.
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
mojom::ArcPrinterInfoPtr ToArcPrinter(const chromeos::Printer& printer,
                                      mojom::PrinterCapabilitiesPtr caps) {
  return mojom::ArcPrinterInfo::New(
      printer.id(), printer.display_name(), mojom::PrinterStatus::IDLE,
      base::Optional<std::string>(printer.description()), base::nullopt,
      std::move(caps));
}

// Transform resolution to Mojo type.
mojom::PrintResolutionPtr ToResolution(const gfx::Size& size) {
  std::stringstream stream;
  stream << size.width() << "x" << size.height();
  std::string name = stream.str();
  return mojom::PrintResolution::New(name, name, size.width(), size.height());
}

// Transform pagper size to Mojo type.
mojom::PrintMediaSizePtr ToMediaSize(
    const printing::PrinterSemanticCapsAndDefaults::Paper& paper) {
  gfx::Size size_mil =
      gfx::ScaleToRoundedSize(paper.size_um, 1.0f / kMicronsPerMil);
  return mojom::PrintMediaSize::New(paper.vendor_id, paper.display_name,
                                    size_mil.width(), size_mil.height());
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
    std::vector<mojom::ArcPrinterInfoPtr> arc_printers;
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
    // TODO
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
    // TODO remove missing printers and only add new ones.
    std::vector<mojom::ArcPrinterInfoPtr> arc_printers;
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
    // TODO PrinterSemanticCapsAndDefaults has no margins, boolean
    // duplex_capable, and unlabeled resolutions.
    if (!caps) {
      RemovePrinter(printer->id());
      return;
    }
    std::vector<mojom::PrintResolutionPtr> resolutions;
    for (const auto& dpi : caps->dpis)
      resolutions.push_back(ToResolution(dpi));

    std::vector<mojom::PrintMediaSizePtr> sizes;
    for (const auto& paper : caps->papers)
      sizes.push_back(ToMediaSize(paper));

    std::vector<mojom::ArcPrinterInfoPtr> arc_printers;
    auto color_modes = static_cast<mojom::PrintColorMode>(0);
    if (caps->bw_model != printing::UNKNOWN_COLOR_MODEL) {
      color_modes = static_cast<mojom::PrintColorMode>(
          static_cast<int>(color_modes) |
          static_cast<int>(mojom::PrintColorMode::MONOCHROME));
    }
    if (caps->color_model != printing::UNKNOWN_COLOR_MODEL) {
      color_modes = static_cast<mojom::PrintColorMode>(
          static_cast<int>(color_modes) |
          static_cast<int>(mojom::PrintColorMode::COLOR));
    }
    mojom::PrintDuplexMode duplex_modes = mojom::PrintDuplexMode::NONE;
    if (caps->duplex_capable) {
      duplex_modes = static_cast<mojom::PrintDuplexMode>(
          static_cast<int>(duplex_modes) |
          static_cast<int>(mojom::PrintDuplexMode::LONG_EDGE) |
          static_cast<int>(mojom::PrintDuplexMode::SHORT_EDGE));
    }
    mojom::PrintDuplexMode default_duplex_mode;
    switch (caps->duplex_default) {
      case printing::LONG_EDGE:
        default_duplex_mode = mojom::PrintDuplexMode::LONG_EDGE;
        break;
      case printing::SHORT_EDGE:
        default_duplex_mode = mojom::PrintDuplexMode::SHORT_EDGE;
        break;
      default:
        default_duplex_mode = mojom::PrintDuplexMode::NONE;
    }
    arc_printers.push_back(ToArcPrinter(
        *printer,
        mojom::PrinterCapabilities::New(
            std::move(sizes), std::move(resolutions),
            mojom::PrintMargins::New(0, 0, 0, 0), color_modes, duplex_modes,
            mojom::ArcPrintAttributes::New(
                ToMediaSize(caps->default_paper),
                ToResolution(caps->default_dpi),
                mojom::PrintMargins::New(0, 0, 0, 0),
                caps->color_default ? mojom::PrintColorMode::COLOR
                                    : mojom::PrintColorMode::MONOCHROME,
                default_duplex_mode))));
    instance_->AddPrinters(std::move(arc_printers));
  }

  // Instance to send responses back to container.
  mojom::PrinterDiscoverySessionInstancePtr instance_;

  // Printers manager for listing available printers.
  std::unique_ptr<chromeos::CupsPrintersManager> printers_manager_;

  // Configurer for selected printers.
  std::unique_ptr<chromeos::PrinterConfigurer> configurer_;

  // Binding that owns |this|.
  mojo::StrongBindingPtr<mojom::PrinterDiscoverySessionHost> binding_;

  // Used in case session is destroyed before response is received.
  base::WeakPtrFactory<DiscoverySession> weak_factory_;
};

// Get requested page ranges from Mojo type
printing::PageRanges FromArcPages(
    const std::vector<mojom::PrintPageRangePtr>& arc_pages) {
  printing::PageRanges pages;
  // Android is weird.
  if (!arc_pages.empty() &&
      arc_pages.back()->end == std::numeric_limits<int>::max()) {
    return pages;
  }
  for (const mojom::PrintPageRangePtr& arc_range : arc_pages) {
    printing::PageRange range;
    range.from = arc_range->start;
    range.to = arc_range->end;
    pages.push_back(range);
  }
  return pages;
}

// Get requested color mode from Mojo type.
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
    : arc_bridge_service_(bridge_service),
      binding_(this),
      profile_(Profile::FromBrowserContext(context)) {
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
                            mojom::ArcPrintJobPtr print_job) {
  instance->Start();

  const mojom::ArcPrintAttributesPtr& attr = print_job->attributes;
  const mojom::PrintMediaSizePtr& arc_media = attr->mediaSize;
  const mojom::PrintResolutionPtr& resolution = attr->resolution;
  const mojom::PrintMarginsPtr& margins = attr->minMargins;
  auto settings = std::make_unique<printing::PrintSettings>();

  gfx::Size size_mils(arc_media->widthMils, arc_media->heightMils);
  printing::PrintSettings::RequestedMedia media;
  media.size_microns = gfx::ScaleToRoundedSize(size_mils, kMicronsPerMil);
  settings->set_requested_media(media);

  // TODO Is it just max(dpm_hor, dpm_ver) as per print_settings_conversion?
  float x_scale = static_cast<float>(resolution->horizontalDpi) / kMilsPerInch;
  float y_scale = static_cast<float>(resolution->verticalDpi) / kMilsPerInch;
  settings->set_dpi_xy(resolution->horizontalDpi, resolution->verticalDpi);

  gfx::Rect area_mils(size_mils);
  area_mils.Inset(margins->leftMils, margins->topMils, margins->rightMils,
                  margins->bottomMils);
  settings->SetPrinterPrintableArea(
      gfx::ScaleToRoundedSize(size_mils, x_scale, y_scale),
      gfx::ScaleToRoundedRect(area_mils, x_scale, y_scale), false);
  if (print_job->printerId) {
    settings->set_device_name(base::UTF8ToUTF16(print_job->printerId.value()));
  }
  settings->set_ranges(FromArcPages(print_job->pages));
  settings->set_title(base::UTF8ToUTF16(print_job->documentName));
  settings->set_color(FromArcColorMode(attr->colorMode));
  settings->set_copies(print_job->copies);
  settings->set_duplex_mode(FromArcDuplexMode(attr->duplexMode));
  mojo::edk::ScopedPlatformHandle scoped_handle;
  PassWrappedPlatformHandle(print_job->data.release().value(), &scoped_handle);
  // ArcPrintRequest manages its own lifetime.
  new ArcPrintRequest(
      std::move(settings), base::File(scoped_handle.release().handle),
      print_job->dataSize,
      base::BindOnce(&mojom::PrintJobInstance::Complete, std::move(instance)));
}

void ArcPrintService::Cancel(mojom::PrintJobInstancePtr instance,
                             mojom::ArcPrintJobPtr print_job) {
  // TODO
}

void ArcPrintService::CreateDiscoverySession(
    mojom::PrinterDiscoverySessionInstancePtr instance,
    CreateDiscoverySessionCallback callback) {
  mojom::PrinterDiscoverySessionHostPtr host_proxy;
  mojo::InterfaceRequest<mojom::PrinterDiscoverySessionHost> request =
      mojo::MakeRequest(&host_proxy);
  std::move(callback).Run(std::move(host_proxy));
  auto session =
      std::make_unique<DiscoverySession>(std::move(instance), profile_);
  DiscoverySession* session_ptr = session.get();
  session_ptr->SetBinding(
      mojo::MakeStrongBinding(std::move(session), std::move(request)));
}

}  // namespace arc
