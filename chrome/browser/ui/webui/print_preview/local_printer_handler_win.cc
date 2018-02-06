// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/print_preview/local_printer_handler_win.h"

#include <string>
#include <utility>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/browser/ui/webui/print_preview/print_preview_utils.h"
#include "chrome/grit/generated_resources.h"
#include "components/printing/service/public/interfaces/print_backend_handler.mojom.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "printing/backend/print_backend.h"
#include "services/service_manager/public/cpp/connector.h"
#include "ui/base/l10n/l10n_util.h"

LocalPrinterHandlerWin::LocalPrinterHandlerWin(
    content::WebContents* preview_web_contents)
    : preview_web_contents_(preview_web_contents) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  StartPrintBackendHandler();
}

LocalPrinterHandlerWin::~LocalPrinterHandlerWin() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  print_backend_handler_.reset();
}

void LocalPrinterHandlerWin::Reset() {
  if (get_default_callback_)
    std::move(get_default_callback_).Run("");
  if (done_callback_)
    std::move(done_callback_).Run();

  added_printers_callback_.Reset();
  for (auto it = capabilities_callbacks_.begin();
       it != capabilities_callbacks_.end(); ++it) {
    std::move(it->second).Run(nullptr);
  }
  capabilities_callbacks_.clear();
  print_backend_handler_.reset();
  StartPrintBackendHandler();
}

void LocalPrinterHandlerWin::GetDefaultPrinter(DefaultPrinterCallback cb) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  get_default_callback_ = std::move(cb);
  DCHECK(!!print_backend_handler_);
  print_backend_handler_->GetDefaultPrinter(base::BindOnce(
      &LocalPrinterHandlerWin::GotDefaultPrinter, base::Unretained(this)));
}

void LocalPrinterHandlerWin::StartGetPrinters(
    const PrinterHandler::AddedPrintersCallback& added_printers_callback,
    PrinterHandler::GetPrintersDoneCallback done_callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  added_printers_callback_ = added_printers_callback;
  done_callback_ = std::move(done_callback);
  DCHECK(!!print_backend_handler_);
  print_backend_handler_->EnumeratePrinters(base::BindOnce(
      &LocalPrinterHandlerWin::GotPrinters, base::Unretained(this)));
}

void LocalPrinterHandlerWin::StartGetCapability(
    const std::string& device_name,
    PrinterHandler::GetCapabilityCallback cb) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  capabilities_callbacks_[device_name] = std::move(cb);
  DCHECK(!!print_backend_handler_);
  print_backend_handler_->FetchCapabilities(
      device_name, base::BindOnce(&LocalPrinterHandlerWin::GotCapabilities,
                                  base::Unretained(this), device_name));
}

void LocalPrinterHandlerWin::StartPrint(
    const std::string& destination_id,
    const std::string& capability,
    const base::string16& job_title,
    const std::string& ticket_json,
    const gfx::Size& page_size,
    const scoped_refptr<base::RefCountedBytes>& print_data,
    PrintCallback callback) {
  // TODO (rbpotter): This will call the printing service also via
  // PdfToEmfConverter. Does it make sense to merge in some way?
  printing::StartLocalPrint(ticket_json, print_data, preview_web_contents_,
                            std::move(callback));
}

void LocalPrinterHandlerWin::GotDefaultPrinter(
    const std::string& printer_name) {
  DCHECK(get_default_callback_);
  std::move(get_default_callback_).Run(printer_name);
}

void LocalPrinterHandlerWin::GotPrinters(const printing::PrinterList& list) {
  printing::ConvertPrinterListForCallback(added_printers_callback_,
                                          std::move(done_callback_), list);
  added_printers_callback_.Reset();
}

void LocalPrinterHandlerWin::GotCapabilities(
    const std::string& device_name,
    std::unique_ptr<base::DictionaryValue> capabilities) {
  std::move(capabilities_callbacks_[device_name]).Run(std::move(capabilities));
  capabilities_callbacks_.erase(device_name);
}

void LocalPrinterHandlerWin::StartPrintBackendHandler() {
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindInterface(printing::mojom::kPrintBackendHandlerServiceName,
                      &print_backend_handler_);
  print_backend_handler_.set_connection_error_handler(
      base::BindOnce(&LocalPrinterHandlerWin::Reset, base::Unretained(this)));
}
