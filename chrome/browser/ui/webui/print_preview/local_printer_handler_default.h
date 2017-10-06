// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_PRINT_PREVIEW_LOCAL_PRINTER_HANDLER_DEFAULT_H_
#define CHROME_BROWSER_UI_WEBUI_PRINT_PREVIEW_LOCAL_PRINTER_HANDLER_DEFAULT_H_

#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "base/values.h"
#include "chrome/browser/ui/webui/print_preview/printer_handler.h"

namespace content {
class WebContents;
}

namespace printing {
class PrintQueriesQueue;
class PrinterQuery;
}  // namespace printing

class LocalPrinterHandlerDefault : public PrinterHandler {
 public:
  explicit LocalPrinterHandlerDefault(
      content::WebContents* preview_web_contents);
  ~LocalPrinterHandlerDefault() override;

  // PrinterHandler implementation.
  void Reset() override;
  void GetDefaultPrinter(const DefaultPrinterCallback& cb) override;
  void StartGetPrinters(const AddedPrintersCallback& added_printers_callback,
                        const GetPrintersDoneCallback& done_callback) override;
  void StartGetCapability(const std::string& destination_id,
                          const GetCapabilityCallback& callback) override;
  // Required by PrinterHandler interface but should never be called.
  void StartPrint(const std::string& destination_id,
                  const std::string& capability,
                  const base::string16& job_title,
                  const std::string& ticket_json,
                  const gfx::Size& page_size,
                  const scoped_refptr<base::RefCountedBytes>& print_data,
                  const PrintCallback& callback) override;

 private:
  void OnPrintSettingsDone(
      const PrintCallback& callback,
      scoped_refptr<printing::PrinterQuery> printer_query,
      const scoped_refptr<base::RefCountedBytes>& print_data);
  void StartPrintJob(const PrintCallback& callback,
                     scoped_refptr<printing::PrinterQuery> printer_query,
                     const scoped_refptr<base::RefCountedBytes>& print_data);

  content::WebContents* preview_web_contents_;
  scoped_refptr<printing::PrintQueriesQueue> queue_;
  base::WeakPtrFactory<LocalPrinterHandlerDefault> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(LocalPrinterHandlerDefault);
};

#endif  // CHROME_BROWSER_UI_WEBUI_PRINT_PREVIEW_LOCAL_PRINTER_HANDLER_DEFAULT_H_
