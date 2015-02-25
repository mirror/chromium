// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_PRINTER_PROVIDER_PRINTER_PROVIDER_API_H_
#define EXTENSIONS_BROWSER_API_PRINTER_PROVIDER_PRINTER_PROVIDER_API_H_

#include <string>

#include "base/callback.h"
#include "components/keyed_service/core/keyed_service.h"

namespace base {
class DictionaryValue;
class ListValue;
}

namespace content {
class BrowserContext;
}

namespace extensions {
struct PrinterProviderPrintJob;
}

namespace extensions {

// Implements chrome.printerProvider API events.
class PrinterProviderAPI : public KeyedService {
 public:
  using GetPrintersCallback =
      base::Callback<void(const base::ListValue& printers, bool done)>;
  using GetCapabilityCallback =
      base::Callback<void(const base::DictionaryValue& capability)>;
  using PrintCallback =
      base::Callback<void(bool success, const std::string& error)>;

  static PrinterProviderAPI* Create(content::BrowserContext* context);

  // Returns generic error string for print request.
  static std::string GetDefaultPrintError();

  // The API currently cannot handle very large files, so the document size that
  // may be sent to an extension is restricted.
  // TODO(tbarzic): Fix the API to support huge documents.
  static const int kMaxDocumentSize = 50 * 1000 * 1000;

  ~PrinterProviderAPI() override {}

  virtual void DispatchGetPrintersRequested(
      const GetPrintersCallback& callback) = 0;

  // Requests printer capability for a printer with id |printer_id|.
  // |printer_id| should be one of the printer ids reported by |GetPrinters|
  // callback.
  // It dispatches chrome.printerProvider.onGetCapabilityRequested event
  // to the extension that manages the printer (which can be determined from
  // |printer_id| value).
  // |callback| is passed a dictionary value containing printer capabilities as
  // reported by the extension.
  virtual void DispatchGetCapabilityRequested(
      const std::string& printer_id,
      const GetCapabilityCallback& callback) = 0;

  // It dispatches chrome.printerProvider.onPrintRequested event with the
  // provided print job. The event is dispatched only to the extension that
  // manages printer with id |job.printer_id|.
  // |callback| is passed the print status returned by the extension, and it
  // must not be null.
  virtual void DispatchPrintRequested(const PrinterProviderPrintJob& job,
                                      const PrintCallback& callback) = 0;
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_PRINTER_PROVIDER_PRINTER_PROVIDER_API_H_
