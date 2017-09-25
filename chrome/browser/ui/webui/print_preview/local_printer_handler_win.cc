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
#include "chrome/common/printing/print_backend_handler.mojom.h"
#include "chrome/common/printing/printer_capabilities.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "services/service_manager/public/cpp/connector.h"
#include "ui/base/l10n/l10n_util.h"

LocalPrinterHandlerWin::LocalPrinterHandlerWin() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  StartPrintBackendHandler();
}

LocalPrinterHandlerWin::~LocalPrinterHandlerWin() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  print_backend_handler_.reset();
}

void LocalPrinterHandlerWin::Reset() {
  if (!get_default_callback_.is_null())
    get_default_callback_.Run("");
  get_default_callback_.Reset();
  if (!done_callback_.is_null()) {
    done_callback_.Run();
  }
  added_printers_callback_.Reset();
  done_callback_.Reset();
  for (auto it = capabilities_callbacks_.begin();
       it != capabilities_callbacks_.end(); ++it) {
    it->second.Run(nullptr);
  }
  capabilities_callbacks_.clear();
  print_backend_handler_.reset();
  StartPrintBackendHandler();
}

void LocalPrinterHandlerWin::GetDefaultPrinter(
    const DefaultPrinterCallback& cb) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  get_default_callback_ = cb;
  DCHECK(!!print_backend_handler_);
  print_backend_handler_->GetDefaultPrinter(base::Bind(
      &LocalPrinterHandlerWin::GotDefaultPrinter, base::Unretained(this)));
}

void LocalPrinterHandlerWin::StartGetPrinters(
    const PrinterHandler::AddedPrintersCallback& added_printers_callback,
    const PrinterHandler::GetPrintersDoneCallback& done_callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  added_printers_callback_ = added_printers_callback;
  done_callback_ = done_callback;
  DCHECK(!!print_backend_handler_);
  print_backend_handler_->EnumeratePrinters(
      base::Bind(&LocalPrinterHandlerWin::GotPrinters, base::Unretained(this)));
}

void LocalPrinterHandlerWin::StartGetCapability(
    const std::string& device_name,
    const PrinterHandler::GetCapabilityCallback& cb) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  capabilities_callbacks_[device_name] = cb;
  DCHECK(!!print_backend_handler_);
  print_backend_handler_->FetchCapabilities(
      device_name, base::Bind(&LocalPrinterHandlerWin::GotCapabilities,
                              base::Unretained(this), device_name));
}

void LocalPrinterHandlerWin::StartPrint(
    const std::string& destination_id,
    const std::string& capability,
    const base::string16& job_title,
    const std::string& ticket_json,
    const gfx::Size& page_size,
    const scoped_refptr<base::RefCountedBytes>& print_data,
    const PrintCallback& callback) {
  NOTREACHED();
}

void LocalPrinterHandlerWin::GotDefaultPrinter(
    const std::string& printer_name) {
  get_default_callback_.Run(printer_name);
  get_default_callback_.Reset();
}

void LocalPrinterHandlerWin::GotPrinters(const printing::PrinterList& list) {
  printing::ConvertPrinterListForCallback(added_printers_callback_,
                                          done_callback_, list);
  done_callback_.Reset();
  added_printers_callback_.Reset();
}

void LocalPrinterHandlerWin::GotCapabilities(
    const std::string& device_name,
    std::unique_ptr<base::DictionaryValue> capabilities) {
  capabilities_callbacks_[device_name].Run(std::move(capabilities));
  capabilities_callbacks_.erase(device_name);
}

void LocalPrinterHandlerWin::StartPrintBackendHandler() {
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindInterface(printing::mojom::kPrintBackendHandlerServiceName,
                      &print_backend_handler_);
  print_backend_handler_.set_connection_error_handler(
      base::Bind(&LocalPrinterHandlerWin::Reset, base::Unretained(this)));
  //  print_backend_handler_.set_disable_sandbox();
}
