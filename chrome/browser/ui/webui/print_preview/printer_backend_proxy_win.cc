// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/print_preview/printer_backend_proxy.h"

#include <string>
#include <utility>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/browser/ui/webui/print_preview/printer_capabilities.h"
#include "chrome/common/print_backend_handler.mojom.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/utility_process_mojo_client.h"
#include "printing/backend/mojo/printing.mojom.h"
#include "printing/backend/print_backend.h"
#include "ui/base/l10n/l10n_util.h"

namespace printing {
// Implementation of PrinterBackendProxy.  Makes calls to the print backend
// via a Mojo utility service.
//

class PrinterBackendProxyWin : public PrinterBackendProxy {
 public:
  PrinterBackendProxyWin() {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    StartPrintBackendHandler();
  }

  ~PrinterBackendProxyWin() override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    print_backend_handler_.reset();
  }

  void GetDefaultPrinter(const DefaultPrinterCallback& cb) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    if (!print_backend_handler_)
      StartPrintBackendHandler();
    print_backend_handler_->service()->GetDefaultPrinter(cb);
  }

  void EnumeratePrinters(const GetPrintersCallback& cb) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    if (!print_backend_handler_)
      StartPrintBackendHandler();
    print_backend_handler_->service()->EnumeratePrinters(base::Bind(
        &PrinterBackendProxyWin::GotPrinters, base::Unretained(this), cb));
  }

  void ConfigurePrinterAndFetchCapabilities(
      const std::string& device_name,
      const PrinterSetupCallback& cb) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    if (!print_backend_handler_)
      StartPrintBackendHandler();
    print_backend_handler_->service()->FetchCapabilities(device_name, cb);
  }

 private:
  void OnConnectionError() { print_backend_handler_.reset(); }

  void StartPrintBackendHandler() {
    print_backend_handler_ = base::MakeUnique<
        content::UtilityProcessMojoClient<chrome::mojom::PrintBackendHandler>>(
        l10n_util::GetStringUTF16(IDS_UTILITY_PROCESS_PRINTING_BACKEND_NAME));
    print_backend_handler_->set_error_callback(base::Bind(
        &PrinterBackendProxyWin::OnConnectionError, base::Unretained(this)));
    print_backend_handler_->set_disable_sandbox();
    print_backend_handler_->Start();
  }

  void GotPrinters(
      const GetPrintersCallback& cb,
      const std::vector<printing::mojom::PrinterBasicInfoPtr> list) {
    PrinterList out;
    for (auto iter = list.begin(); iter != list.end(); ++iter) {
      printing::PrinterBasicInfo info;
      info.printer_name = (*iter)->printer_name;
      info.printer_description = (*iter)->printer_description;
      info.is_default = (*iter)->is_default;
      info.printer_status = (*iter)->printer_status;
      for (auto option : (*iter)->options)
        info.options[option.first] = option.second;
      out.push_back(info);
    }
    cb.Run(out);
  }

  std::unique_ptr<
      content::UtilityProcessMojoClient<chrome::mojom::PrintBackendHandler>>
      print_backend_handler_;
  DISALLOW_COPY_AND_ASSIGN(PrinterBackendProxyWin);
};

// static
std::unique_ptr<PrinterBackendProxy> PrinterBackendProxy::Create() {
  return base::MakeUnique<PrinterBackendProxyWin>();
}

}  // namespace printing
