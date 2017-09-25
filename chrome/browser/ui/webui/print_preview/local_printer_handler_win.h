// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_PRINT_PREVIEW_LOCAL_PRINTER_HANDLER_WIN_H_
#define CHROME_BROWSER_UI_WEBUI_PRINT_PREVIEW_LOCAL_PRINTER_HANDLER_WIN_H_

#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "base/values.h"
#include "chrome/browser/ui/webui/print_preview/printer_handler.h"
#include "chrome/common/printing/print_backend_handler.mojom.h"
#include "content/public/browser/utility_process_mojo_client.h"

class LocalPrinterHandlerWin : public PrinterHandler {
 public:
  LocalPrinterHandlerWin();
  ~LocalPrinterHandlerWin() override;

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
  void GotDefaultPrinter(const std::string& printer_name);
  void GotPrinters(const printing::PrinterList& list);
  void GotCapabilities(const std::string& device_name,
                       std::unique_ptr<base::DictionaryValue> capabilities);
  void StartPrintBackendHandler();

  DefaultPrinterCallback get_default_callback_;
  AddedPrintersCallback added_printers_callback_;
  GetPrintersDoneCallback done_callback_;
  std::map<std::string, GetCapabilityCallback> capabilities_callbacks_;
  mojo::InterfacePtr<printing::mojom::PrintBackendHandler>
      print_backend_handler_;

  DISALLOW_COPY_AND_ASSIGN(LocalPrinterHandlerWin);
};

#endif  // CHROME_BROWSER_UI_WEBUI_PRINT_PREVIEW_LOCAL_PRINTER_HANDLER_WIN_H_
