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
#include "printing/backend/print_backend.h"
#include "printing/backend/print_backend_consts.h"
#include "printing/pdf_metafile_skia.h"
#include "printing/print_job_constants.h"
#include "printing/printed_document.h"
#include "printing/printed_pages_source.h"

namespace arc {
namespace {

const float kMicronsPerMil = 25.4f;
const int kMilsPerInch = 1000;

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

class ArcPrintRequest : public content::NotificationObserver {
 public:
  ArcPrintRequest(std::unique_ptr<printing::PrintSettings> settings,
                  base::File file,
                  size_t data_size,
                  int page_count,
                  const gfx::Size& page_size,
                  const gfx::Rect& area,
                  base::OnceClosure done_callback)
      : settings_(std::move(settings)),
        done_callback_(std::move(done_callback)),
        file_(std::move(file)),
        data_size_(data_size),
        page_count_(page_count),
        page_size_(page_size),
        area_(area) {
    base::PostTaskWithTraits(
        FROM_HERE, {base::MayBlock()},
        base::BindOnce(&ArcPrintRequest::ReadFile, base::Unretained(this)));
    content::BrowserThread::PostTask(
        content::BrowserThread::IO, FROM_HERE,
        base::BindOnce(&ArcPrintRequest::SetSettings, base::Unretained(this)));
  }

  // content::NotificationObserver implementation.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override {
    const printing::JobEventDetails& event_details =
        *content::Details<printing::JobEventDetails>(details).ptr();
    // TODO other types
    if (event_details.type() == printing::JobEventDetails::JOB_DONE) {
      std::move(done_callback_).Run();
      delete this;
    }
  }

 private:
  // Call on blocking thread.
  void ReadFile() {
    // TODO Can we make give pipe to Skia directly?
    std::vector<char> buf(data_size_);
    ssize_t bytes = file_.ReadAtCurrentPos(buf.data(), data_size_);
    if (bytes < 0) {
      LOG(ERROR) << "Error reading PDF: " << strerror(errno);
    }
    DCHECK((size_t)bytes == data_size_);
    file_.Close();

    auto metafile = std::make_unique<printing::PdfMetafileSkia>(
        printing::SkiaDocumentType::PDF);
    metafile->InitFromData(buf.data(), data_size_);
    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::BindOnce(&ArcPrintRequest::OnFileRead, base::Unretained(this),
                       base::Passed(&metafile)));
  }

  // Call on UI.
  void OnFileRead(std::unique_ptr<printing::PdfMetafileSkia> metafile) {
    metafile_ = std::move(metafile);
    StartPrintingIfReady();
  }

  // Call on IO.
  void SetSettings() {
    query_ = base::MakeRefCounted<printing::PrinterQuery>(
        content::ChildProcessHost::kInvalidUniqueID,
        content::ChildProcessHost::kInvalidUniqueID);
    query_->SetSettings(
        std::move(settings_),
        base::Bind(&ArcPrintRequest::SetSettingsDone, base::Unretained(this)));
  }

  // Call on IO.
  void SetSettingsDone() {
    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::BindOnce(&ArcPrintRequest::SetSettingsDoneOnUI,
                       base::Unretained(this)));
  }

  // Call on UI.
  void SetSettingsDoneOnUI() {
    ArcPagesSource source;
    job_ = base::MakeRefCounted<printing::PrintJob>();
    job_->Initialize(query_.get(), &source, page_count_);
    job_->DisconnectSource();
    registrar_.Add(this, chrome::NOTIFICATION_PRINT_JOB_EVENT,
                   content::Source<printing::PrintJob>(job_.get()));
    StartPrintingIfReady();
  }

  // Call on UI.
  void StartPrintingIfReady() {
    if (job_ && metafile_) {
      printing::PrintedDocument* document = job_->document();
      document->SetPage(0, std::move(metafile_), page_size_, area_);
      job_->StartPrinting();
    }
  }

  // Modified on IO.
  std::unique_ptr<printing::PrintSettings> settings_;
  scoped_refptr<printing::PrinterQuery> query_;

  // Modified on UI.
  std::unique_ptr<printing::PdfMetafileSkia> metafile_;
  scoped_refptr<printing::PrintJob> job_;
  content::NotificationRegistrar registrar_;
  base::OnceClosure done_callback_;

  // Modified on blocking thread.
  base::File file_;

  const size_t data_size_;
  const int page_count_;
  const gfx::Size page_size_;
  const gfx::Rect area_;
};

std::unique_ptr<printing::PrinterSemanticCapsAndDefaults> FetchCapabilities(
    const std::string& printer_id) {
  scoped_refptr<printing::PrintBackend> backend(
      printing::PrintBackend::CreateInstance(nullptr));
  auto caps = std::make_unique<printing::PrinterSemanticCapsAndDefaults>();
  if (!backend->GetPrinterSemanticCapsAndDefaults(printer_id, caps.get())) {
    LOG(ERROR) << "Failed to get caps for " << printer_id;
    return nullptr;
  }
  return caps;
}

mojom::ArcPrinterInfoPtr ToArcPrinter(const chromeos::Printer& printer,
                                      mojom::PrinterCapabilitiesPtr caps) {
  return mojom::ArcPrinterInfo::New(
      printer.id(), printer.display_name(), mojom::PrinterStatus::IDLE,
      base::Optional<std::string>(printer.description()), base::nullopt,
      std::move(caps));
}

mojom::PrintMediaSizePtr ToMediaSize(
    const printing::PrinterSemanticCapsAndDefaults::Paper& paper) {
  gfx::Size size_mil =
      gfx::ScaleToRoundedSize(paper.size_um, 1.0f / kMicronsPerMil);
  return mojom::PrintMediaSize::New(paper.vendor_id, paper.display_name,
                                    size_mil.width(), size_mil.height());
}

mojom::PrintResolutionPtr ToResolution(const gfx::Size& size) {
  std::stringstream stream;
  stream << size.width() << "x" << size.height();
  std::string name = stream.str();
  return mojom::PrintResolution::New(name, name, size.width(), size.height());
}

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

printing::ColorModel FromArcColorMode(mojom::PrintColorMode mode) {
  switch (mode) {
    case mojom::PrintColorMode::MONOCHROME:
      return printing::GRAY;
    case mojom::PrintColorMode::COLOR:
      return printing::COLOR;
  }
  NOTREACHED();
}

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
    : arc_bridge_service_(bridge_service), binding_(this), weak_factory_(this) {
  Profile* profile = Profile::FromBrowserContext(context);
  printers_manager_ = chromeos::CupsPrintersManager::Create(profile);
  configurer_ = chromeos::PrinterConfigurer::Create(profile);
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
  auto* instance =
      ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->print(), StartJob);
  instance->StartJob(print_job->id);

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
  float x_scale = (float)resolution->horizontalDpi / kMilsPerInch;
  float y_scale = (float)resolution->verticalDpi / kMilsPerInch;
  settings->set_dpi_xy(resolution->horizontalDpi, resolution->verticalDpi);

  gfx::Size size = gfx::ScaleToRoundedSize(size_mils, x_scale, y_scale);
  gfx::Rect area(size_mils);
  area.Inset(margins->leftMils, margins->topMils, margins->rightMils,
             margins->bottomMils);
  area = gfx::ScaleToRoundedRect(area, x_scale, y_scale);

  settings->SetPrinterPrintableArea(size, area, false);
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
  new ArcPrintRequest(
      std::move(settings), base::File(scoped_handle.release().handle),
      print_job->dataSize, print_job->documentPageCount, size, area,
      base::BindOnce(&ArcPrintService::PrintingDone, weak_factory_.GetWeakPtr(),
                     print_job->id));
}

void ArcPrintService::PrintingDone(const std::vector<int8_t>& job_id) {
  auto* instance =
      ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->print(), CompleteJob);
  instance->CompleteJob(job_id);
}

void ArcPrintService::Cancel(mojom::ArcPrintJobPtr print_job) {
  // TODO
}

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
    for (const auto& printer : printers)
      arc_printers.push_back(ToArcPrinter(printer, nullptr));
  }
  if (!arc_printers.empty())
    instance->AddPrinters(session_id, std::move(arc_printers));
}

void ArcPrintService::StopPrinterDiscovery(const std::string& session_id) {
  discovering_sessions_.insert(session_id);
}

void ArcPrintService::ValidatePrinters(
    const std::string& session_id,
    const std::vector<std::string>& printer_ids) {}

void ArcPrintService::StartPrinterStateTracking(const std::string& session_id,
                                                const std::string& printer_id) {
  std::unique_ptr<chromeos::Printer> printer =
      printers_manager_->GetPrinter(printer_id);
  if (!printer) {
    RemovePrinter(session_id, printer_id);
    CapabilitiesReceived(session_id, std::move(printer), nullptr);
  }
  if (printers_manager_->IsPrinterInstalled(*printer)) {
    PrinterInstalled(session_id, std::move(printer), chromeos::kSuccess);
    return;
  }
  const chromeos::Printer& printer_ref = *printer;
  configurer_->SetUpPrinter(
      printer_ref,
      base::Bind(&ArcPrintService::PrinterInstalled, weak_factory_.GetWeakPtr(),
                 session_id, base::Passed(&printer)));
}

void ArcPrintService::RemovePrinter(const std::string& session_id,
                                    const std::string& printer_id) {
  auto* instance =
      ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->print(), RemovePrinters);
  instance->RemovePrinters(session_id, std::vector<std::string>{printer_id});
}

void ArcPrintService::PrinterInstalled(
    const std::string& session_id,
    std::unique_ptr<chromeos::Printer> printer,
    chromeos::PrinterSetupResult result) {
  if (result != chromeos::kSuccess) {
    RemovePrinter(session_id, printer->id());
    return;
  }
  printers_manager_->PrinterInstalled(*printer);
  const std::string& printer_id = printer->id();
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&FetchCapabilities, printer_id),
      base::BindOnce(&ArcPrintService::CapabilitiesReceived,
                     weak_factory_.GetWeakPtr(), session_id,
                     std::move(printer)));
}

void ArcPrintService::CapabilitiesReceived(
    const std::string& session_id,
    std::unique_ptr<chromeos::Printer> printer,
    std::unique_ptr<printing::PrinterSemanticCapsAndDefaults> caps) {
  // TODO PrinterSemanticCapsAndDefaults has no margins, boolean duplex_capable,
  // and unlabeled resolutions.
  if (!caps) {
    RemovePrinter(session_id, printer->id());
    return;
  }
  auto* instance =
      ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->print(), AddPrinters);

  std::vector<mojom::PrintResolutionPtr> resolutions;
  for (const auto& dpi : caps->dpis)
    resolutions.push_back(ToResolution(dpi));

  std::vector<mojom::PrintMediaSizePtr> sizes;
  for (const auto& paper : caps->papers)
    sizes.push_back(ToMediaSize(paper));

  std::vector<mojom::ArcPrinterInfoPtr> arc_printers;
  auto color_modes = (mojom::PrintColorMode)0;
  if (caps->bw_model != printing::UNKNOWN_COLOR_MODEL) {
    color_modes = (mojom::PrintColorMode)(
        (int)color_modes | (int)mojom::PrintColorMode::MONOCHROME);
  }
  if (caps->color_model != printing::UNKNOWN_COLOR_MODEL) {
    color_modes = (mojom::PrintColorMode)((int)color_modes |
                                          (int)mojom::PrintColorMode::COLOR);
  }
  mojom::PrintDuplexMode duplex_modes = mojom::PrintDuplexMode::NONE;
  if (caps->duplex_capable) {
    duplex_modes = (mojom::PrintDuplexMode)(
        (int)duplex_modes | (int)mojom::PrintDuplexMode::LONG_EDGE |
        (int)mojom::PrintDuplexMode::SHORT_EDGE);
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
              ToMediaSize(caps->default_paper), ToResolution(caps->default_dpi),
              mojom::PrintMargins::New(0, 0, 0, 0),
              caps->color_default ? mojom::PrintColorMode::COLOR
                                  : mojom::PrintColorMode::MONOCHROME,
              default_duplex_mode))));
  instance->AddPrinters(session_id, std::move(arc_printers));
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
  if (discovering_sessions_.empty())
    return;

  auto* instance =
      ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->print(), AddPrinters);
  std::vector<mojom::ArcPrinterInfoPtr> arc_printers;
  for (const auto& printer : printers)
    arc_printers.push_back(ToArcPrinter(printer, nullptr));

  for (auto it = discovering_sessions_.begin();
       it != discovering_sessions_.end(); ++it) {
    if (it == discovering_sessions_.rbegin().base()) {
      instance->AddPrinters(*it, std::move(arc_printers));
    } else {
      std::vector<mojom::ArcPrinterInfoPtr> arc_printers_copy;
      for (const auto& arc_printer : arc_printers)
        arc_printers_copy.push_back(arc_printer->Clone());

      instance->AddPrinters(*it, std::move(arc_printers_copy));
    }
  }
}

}  // namespace arc
