// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_PRINT_PREVIEW_LOCAL_PRINTER_HANDLER_CHROMEOS_H_
#define CHROME_BROWSER_UI_WEBUI_PRINT_PREVIEW_LOCAL_PRINTER_HANDLER_CHROMEOS_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "chrome/browser/chromeos/printing/cups_printers_manager.h"
#include "chrome/browser/chromeos/printing/printer_configurer.h"
#include "chrome/browser/ui/webui/print_preview/printer_handler.h"
#include "chromeos/printing/printer_configuration.h"

namespace content {
class WebContents;
}

namespace printing {
class PrintQueriesQueue;
class PrinterQuery;
}  // namespace printing

class Profile;

class LocalPrinterHandlerChromeos : public PrinterHandler {
 public:
  LocalPrinterHandlerChromeos(Profile* profile,
                              content::WebContents* preview_web_contents);
  ~LocalPrinterHandlerChromeos() override;

  // PrinterHandler implementation
  void Reset() override;
  void GetDefaultPrinter(const DefaultPrinterCallback& cb) override;
  void StartGetPrinters(const AddedPrintersCallback& added_printers_callback,
                        const GetPrintersDoneCallback& done_callback) override;
  void StartGetCapability(const std::string& printer_name,
                          const GetCapabilityCallback& cb) override;
  // Required by PrinterHandler interface but should never be called.
  void StartPrint(const std::string& destination_id,
                  const std::string& capability,
                  const base::string16& job_title,
                  const std::string& ticket_json,
                  const gfx::Size& page_size,
                  const scoped_refptr<base::RefCountedBytes>& print_data,
                  const PrintCallback& callback) override;

 private:
  void HandlePrinterSetup(std::unique_ptr<chromeos::Printer> printer,
                          const GetCapabilityCallback& cb,
                          chromeos::PrinterSetupResult result);
  void OnPrintSettingsDone(
      const PrintCallback& callback,
      scoped_refptr<printing::PrinterQuery> printer_query,
      const scoped_refptr<base::RefCountedBytes>& print_data);
  void StartPrintJob(const PrintCallback& callback,
                     scoped_refptr<printing::PrinterQuery> printer_query,
                     const scoped_refptr<base::RefCountedBytes>& print_data);

  content::WebContents* preview_web_contents_;
  scoped_refptr<printing::PrintQueriesQueue> queue_;
  std::unique_ptr<chromeos::CupsPrintersManager> printers_manager_;
  scoped_refptr<chromeos::PpdProvider> ppd_provider_;
  std::unique_ptr<chromeos::PrinterConfigurer> printer_configurer_;
  base::WeakPtrFactory<LocalPrinterHandlerChromeos> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(LocalPrinterHandlerChromeos);
};

#endif  // CHROME_BROWSER_UI_WEBUI_PRINT_PREVIEW_LOCAL_PRINTER_HANDLER_CHROMEOS_H_
