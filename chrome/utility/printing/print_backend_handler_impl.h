// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_UTILITY_PRINTING_PRINT_BACKEND_HANDLER_IMPL_H_
#define CHROME_UTILITY_PRINTING_PRINT_BACKEND_HANDLER_IMPL_H_

#include "chrome/common/print_backend_handler.mojom.h"

namespace printing {

class PrintBackendHandlerImpl : public chrome::mojom::PrintBackendHandler {
 public:
  PrintBackendHandlerImpl();
  ~PrintBackendHandlerImpl() override;
  static void Create(chrome::mojom::PrintBackendHandlerRequest request);

 private:
  // chrome::mojom::PrintBackendHandler
  void GetDefaultPrinter(const GetDefaultPrinterCallback& callback) override;
  void EnumeratePrinters(const EnumeratePrintersCallback& callback) override;
  void FetchCapabilities(const std::string& device_name,
                         const FetchCapabilitiesCallback& callback) override;

  DISALLOW_COPY_AND_ASSIGN(PrintBackendHandlerImpl);
};

}  // namespace printing

#endif  // CHROME_UTILITY_PRINTING_PRINT_BACKEND_HANDLER_IMPL_H_
