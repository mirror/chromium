// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/print_preview/local_printer_handler_default.h"

#include <string>
#include <utility>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/weak_ptr.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/printing/print_job_manager.h"
#include "chrome/browser/printing/print_preview_dialog_controller.h"
#include "chrome/browser/printing/print_view_manager.h"
#include "chrome/browser/printing/printer_query.h"
#include "chrome/browser/ui/webui/print_preview/print_preview_ui.h"
#include "chrome/browser/ui/webui/print_preview/printer_capabilities.h"
#include "components/printing/common/print_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "printing/backend/print_backend.h"
#include "printing/page_range.h"
#include "printing/print_settings.h"

namespace {

printing::PrinterList EnumeratePrintersAsync() {
  base::ThreadRestrictions::AssertIOAllowed();
  scoped_refptr<printing::PrintBackend> print_backend(
      printing::PrintBackend::CreateInstance(nullptr));

  printing::PrinterList printer_list;
  print_backend->EnumeratePrinters(&printer_list);
  return printer_list;
}

std::unique_ptr<base::DictionaryValue> FetchCapabilitiesAsync(
    const std::string& device_name) {
  base::ThreadRestrictions::AssertIOAllowed();
  scoped_refptr<printing::PrintBackend> print_backend(
      printing::PrintBackend::CreateInstance(nullptr));

  VLOG(1) << "Get printer capabilities start for " << device_name;

  if (!print_backend->IsValidPrinter(device_name)) {
    LOG(WARNING) << "Invalid printer " << device_name;
    return nullptr;
  }

  printing::PrinterBasicInfo basic_info;
  if (!print_backend->GetPrinterBasicInfo(device_name, &basic_info)) {
    return nullptr;
  }

  return printing::GetSettingsOnBlockingPool(device_name, basic_info);
}

std::string GetDefaultPrinterAsync() {
  base::ThreadRestrictions::AssertIOAllowed();
  scoped_refptr<printing::PrintBackend> print_backend(
      printing::PrintBackend::CreateInstance(nullptr));

  std::string default_printer = print_backend->GetDefaultPrinterName();
  VLOG(1) << "Default Printer: " << default_printer;
  return default_printer;
}

}  // namespace

LocalPrinterHandlerDefault::LocalPrinterHandlerDefault(
    content::WebContents* preview_web_contents)
    : preview_web_contents_(preview_web_contents),
      queue_(g_browser_process->print_job_manager()->queue()),
      weak_ptr_factory_(this) {}

LocalPrinterHandlerDefault::~LocalPrinterHandlerDefault() {}

void LocalPrinterHandlerDefault::Reset() {
  weak_ptr_factory_.InvalidateWeakPtrs();
}

void LocalPrinterHandlerDefault::GetDefaultPrinter(
    const DefaultPrinterCallback& cb) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::Bind(&GetDefaultPrinterAsync), cb);
}

void LocalPrinterHandlerDefault::StartGetPrinters(
    const AddedPrintersCallback& callback,
    const GetPrintersDoneCallback& done_callback) {
  VLOG(1) << "Enumerate printers start";
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::Bind(&EnumeratePrintersAsync),
      base::Bind(&printing::ConvertPrinterListForCallback, callback,
                 done_callback));
}

void LocalPrinterHandlerDefault::StartGetCapability(
    const std::string& device_name,
    const GetCapabilityCallback& cb) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::Bind(&FetchCapabilitiesAsync, device_name), cb);
}

void LocalPrinterHandlerDefault::StartPrint(
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
      base::Bind(&LocalPrinterHandlerDefault::OnPrintSettingsDone,
                 weak_ptr_factory_.GetWeakPtr(), callback, printer_query,
                 print_data));
}

void LocalPrinterHandlerDefault::OnPrintSettingsDone(
    const PrintCallback& callback,
    scoped_refptr<printing::PrinterQuery> printer_query,
    const scoped_refptr<base::RefCountedBytes>& print_data) {
  queue_->QueuePrinterQuery(printer_query.get());

  // Post task so that the query has time to reset the callback before calling
  // OnDidGetPrintedPagesCount.
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&LocalPrinterHandlerDefault::StartPrintJob,
                 weak_ptr_factory_.GetWeakPtr(), callback, printer_query,
                 print_data));
}

void LocalPrinterHandlerDefault::StartPrintJob(
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
#if defined(OS_WIN)
    params.page_size = settings.page_setup_device_units().physical_size();
    params.content_area =
        gfx::Rect(0, 0, params.page_size.width(), params.page_size.height());
    params.physical_offsets =
        gfx::Point(settings.page_setup_device_units().content_area().x(),
                   settings.page_setup_device_units().content_area().y());
#endif
    print_view_manager->OnDidPrintPage(params);
  }
  callback.Run(true, base::Value());
}
