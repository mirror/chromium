// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/print_preview/local_printer_handler_chromeos.h"

#include <memory>
#include <vector>

#include "base/bind_helpers.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/weak_ptr.h"
#include "base/metrics/histogram_macros.h"
#include "base/task_scheduler/post_task.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/printing/cups_print_job_manager.h"
#include "chrome/browser/chromeos/printing/cups_print_job_manager_factory.h"
#include "chrome/browser/chromeos/printing/cups_printers_manager.h"
#include "chrome/browser/chromeos/printing/ppd_provider_factory.h"
#include "chrome/browser/chromeos/printing/printer_configurer.h"
#include "chrome/browser/printing/print_job_manager.h"
#include "chrome/browser/printing/print_preview_dialog_controller.h"
#include "chrome/browser/printing/print_view_manager.h"
#include "chrome/browser/printing/printer_query.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/print_preview/print_preview_ui.h"
#include "chrome/browser/ui/webui/print_preview/printer_capabilities.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/debug_daemon_client.h"
#include "chromeos/printing/ppd_provider.h"
#include "chromeos/printing/printer_configuration.h"
#include "components/printing/common/print_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "printing/backend/print_backend_consts.h"
#include "printing/page_range.h"
#include "printing/print_settings.h"

namespace {

using chromeos::CupsPrintersManager;

// Store the name used in CUPS, Printer#id in |printer_name|, the description
// as the system_driverinfo option value, and the Printer#display_name in
// the |printer_description| field.  This will match how Mac OS X presents
// printer information.
printing::PrinterBasicInfo ToBasicInfo(const chromeos::Printer& printer) {
  printing::PrinterBasicInfo basic_info;

  // TODO(skau): Unify Mac with the other platforms for display name
  // presentation so I can remove this strange code.
  basic_info.options[kDriverInfoTagName] = printer.description();
  basic_info.options[kCUPSEnterprisePrinter] =
      (printer.source() == chromeos::Printer::SRC_POLICY) ? kValueTrue
                                                          : kValueFalse;
  basic_info.printer_name = printer.id();
  basic_info.printer_description = printer.display_name();
  return basic_info;
}

void AddPrintersToList(const std::vector<chromeos::Printer>& printers,
                       printing::PrinterList* list) {
  for (const auto& printer : printers) {
    list->push_back(ToBasicInfo(printer));
  }
}

void FetchCapabilities(std::unique_ptr<chromeos::Printer> printer,
                       const PrinterHandler::GetCapabilityCallback& cb) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  printing::PrinterBasicInfo basic_info = ToBasicInfo(*printer);

  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::Bind(&printing::GetSettingsOnBlockingPool, printer->id(),
                 basic_info),
      cb);
}

}  // namespace

LocalPrinterHandlerChromeos::LocalPrinterHandlerChromeos(
    Profile* profile,
    content::WebContents* preview_web_contents)
    : preview_web_contents_(preview_web_contents),
      queue_(g_browser_process->print_job_manager()->queue()),
      printers_manager_(CupsPrintersManager::Create(profile)),
      printer_configurer_(chromeos::PrinterConfigurer::Create(profile)),
      weak_factory_(this) {
  // Construct the CupsPrintJobManager to listen for printing events.
  chromeos::CupsPrintJobManagerFactory::GetForBrowserContext(profile);
}

LocalPrinterHandlerChromeos::~LocalPrinterHandlerChromeos() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

void LocalPrinterHandlerChromeos::Reset() {
  weak_factory_.InvalidateWeakPtrs();
}

void LocalPrinterHandlerChromeos::GetDefaultPrinter(
    const DefaultPrinterCallback& cb) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  // TODO(crbug.com/660898): Add default printers to ChromeOS.

  content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
                                   base::Bind(cb, ""));
}

void LocalPrinterHandlerChromeos::StartGetPrinters(
    const AddedPrintersCallback& added_printers_callback,
    const GetPrintersDoneCallback& done_callback) {
  // SyncedPrintersManager is not thread safe and must be called from the UI
  // thread.
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  printing::PrinterList printer_list;
  AddPrintersToList(
      printers_manager_->GetPrinters(CupsPrintersManager::kConfigured),
      &printer_list);
  AddPrintersToList(
      printers_manager_->GetPrinters(CupsPrintersManager::kEnterprise),
      &printer_list);
  AddPrintersToList(
      printers_manager_->GetPrinters(CupsPrintersManager::kAutomatic),
      &printer_list);

  printing::ConvertPrinterListForCallback(added_printers_callback,
                                          done_callback, printer_list);
}

void LocalPrinterHandlerChromeos::StartGetCapability(
    const std::string& printer_name,
    const GetCapabilityCallback& cb) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  std::unique_ptr<chromeos::Printer> printer =
      printers_manager_->GetPrinter(printer_name);
  if (!printer) {
    // If the printer was removed, the lookup will fail.
    content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
                                     base::Bind(cb, nullptr));
    return;
  }

  // Log printer configuration for selected printer.
  UMA_HISTOGRAM_ENUMERATION("Printing.CUPS.ProtocolUsed",
                            printer->GetProtocol(),
                            chromeos::Printer::kProtocolMax);

  if (printers_manager_->IsPrinterInstalled(*printer)) {
    // Skip setup if the printer is already installed.
    HandlePrinterSetup(std::move(printer), cb, chromeos::kSuccess);
    return;
  }

  const chromeos::Printer& printer_ref = *printer;
  printer_configurer_->SetUpPrinter(
      printer_ref,
      base::Bind(&LocalPrinterHandlerChromeos::HandlePrinterSetup,
                 weak_factory_.GetWeakPtr(), base::Passed(&printer), cb));
}

void LocalPrinterHandlerChromeos::StartPrint(
    const std::string& destination_id,
    const std::string& capability,
    const base::string16& job_title,
    const std::string& ticket_json,
    const gfx::Size& page_size,
    const scoped_refptr<base::RefCountedBytes>& print_data,
    const PrintCallback& callback) {
  NOTREACHED();
}

void LocalPrinterHandlerChromeos::HandlePrinterSetup(
    std::unique_ptr<chromeos::Printer> printer,
    const GetCapabilityCallback& cb,
    chromeos::PrinterSetupResult result) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  switch (result) {
    case chromeos::PrinterSetupResult::kSuccess:
      VLOG(1) << "Printer setup successful for " << printer->id()
              << " fetching properties";
      printers_manager_->PrinterInstalled(*printer);

      // fetch settings on the blocking pool and invoke callback.
      FetchCapabilities(std::move(printer), cb);
      return;
    case chromeos::PrinterSetupResult::kPpdNotFound:
      LOG(WARNING) << "Could not find PPD.  Check printer configuration.";
      // Prompt user to update configuration.
      // TODO(skau): Fill me in
      break;
    case chromeos::PrinterSetupResult::kPpdUnretrievable:
      LOG(WARNING) << "Could not download PPD.  Check Internet connection.";
      // Could not download PPD.  Connect to Internet.
      // TODO(skau): Fill me in
      break;
    case chromeos::PrinterSetupResult::kPrinterUnreachable:
    case chromeos::PrinterSetupResult::kDbusError:
    case chromeos::PrinterSetupResult::kPpdTooLarge:
    case chromeos::PrinterSetupResult::kInvalidPpd:
    case chromeos::PrinterSetupResult::kFatalError:
      LOG(ERROR) << "Unexpected error in printer setup." << result;
      break;
    case chromeos::PrinterSetupResult::kMaxValue:
      NOTREACHED() << "This value is not expected";
      break;
  }

  // TODO(skau): Open printer settings if this is resolvable.
  cb.Run(nullptr);
}

// TODO: factor this out so chromeos and default share code.
void LocalPrinterHandlerChromeos::StartPrint(
    const std::string& destination_id,
    const std::string& capability,
    const base::string16& job_title,
    const std::string& ticket_json,
    const gfx::Size& page_size,
    const scoped_refptr<base::RefCountedBytes>& print_data,
    const PrintCallback& callback) {
  std::unique_ptr<base::DictionaryValue> job_settings =
      base::DictionaryValue::From(base::JSONReader::Read(ticket_json));
  if (!job_settings) {
    callback.Run(false, base::Value("Invalid settings"));
    return;
  }
  job_settings->SetBoolean(printing::kSettingHeaderFooterEnabled, false);
  job_settings->SetInteger(printing::kSettingMarginsType, printing::NO_MARGINS);
  scoped_refptr<printing::PrinterQuery> printer_query =
      queue_->CreatePrinterQuery(
          preview_web_contents_->GetMainFrame()->GetProcess()->GetID(),
          preview_web_contents_->GetMainFrame()->GetRoutingID());
  printer_query->SetSettings(
      std::move(job_settings),
      base::Bind(&LocalPrinterHandlerChromeos::OnPrintSettingsDone,
                 weak_ptr_factory_.GetWeakPtr(), callback, printer_query,
                 print_data));
}

void LocalPrinterHandlerChromeos::OnPrintSettingsDone(
    const PrintCallback& callback,
    scoped_refptr<printing::PrinterQuery> printer_query,
    const scoped_refptr<base::RefCountedBytes>& print_data) {
  queue_->QueuePrinterQuery(printer_query.get());

  // Post task so that the query has time to reset the callback before calling
  // OnDidGetPrintedPagesCount.
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&LocalPrinterHandlerChromeos::StartPrintJob,
                 weak_ptr_factory_.GetWeakPtr(), callback, printer_query));
}

void LocalPrinterHandlerChromeos::StartPrintJob(
    const PrintCallback& callback,
    scoped_refptr<printing::PrinterQuery> printer_query,
    const scoped_refptr<base::RefCountedBytes>& print_data) {
  const printing::PrintSettings& settings = printer_query->settings();
  // Get page numbers
  std::vector<int> page_nums = printing::PageRange::GetPages(settings.ranges());
  if (page_nums.empty()) {  // Print the whole document.
    PrintPreviewUI* print_preview_ui =
        static_cast<PrintPreviewUI*>(preview_web_contents_->GetController());
    for (int i = 0; i < print_preview_ui->GetAvailableDraftPageCount(); i++)
      page_nums.push_back(i);
  }

  // Get print view manager.
  printing::PrintPreviewDialogController* dialog_controller =
      printing::PrintPreviewDialogController::GetInstance();
  content::WebContents* initiator =
      dialog_controller ? dialog_controller->GetInitiator(preview_web_contents_)
                        : nullptr;
  printing::PrintViewManager* print_view_manager =
      printing::PrintViewManager::FromWebContents(initiator);
  if (!print_view_manager) {
    callback.Run(false, base::Value("Initiator closed"));
    return;
  }
  print_view_manager->OnDidGetPrintedPagesCount(printer_query->cookie(),
                                                page_nums.size());
  // Print pages
  for (int page_num : page_nums) {
    PrintHostMsg_DidPrintPage_Params params;
    if (page_num == page_nums[0]) {
      // Copy the metafile data to a shared buffer.
      std::unique_ptr<base::SharedMemory> shared_buf(new base::SharedMemory());
      if (!shared_buf ||
          !shared_buf->CreateAndMapAnonymous(print_data->size())) {
        callback.Run(false, base::Value("Print failed"));
        return;
      }
      memcpy(shared_buf->memory(), &(print_data->data()[0]),
             print_data->size());
      params.metafile_data_handle =
          base::SharedMemory::DuplicateHandle(shared_buf->handle());
    }
    params.data_size = print_data->size();
    params.page_number = page_num;
    params.document_cookie = printer_query->cookie();
    print_view_manager->OnDidPrintPage(params);
  }
  callback.Run(true, base::Value());
}
