// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PRINTING_SERVICE_PRINT_BACKEND_HANDLER_IMPL_H_
#define COMPONENTS_PRINTING_SERVICE_PRINT_BACKEND_HANDLER_IMPL_H_

#include "components/printing/service/public/interfaces/print_backend_handler.mojom.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace printing {

class PrintBackend;

class PrintBackendHandlerImpl : public printing::mojom::PrintBackendHandler {
 public:
  explicit PrintBackendHandlerImpl(
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);

  ~PrintBackendHandlerImpl() override;

 private:
  // chrome::mojom::PrintBackendHandler
  void GetDefaultPrinter(GetDefaultPrinterCallback callback) override;
  void EnumeratePrinters(EnumeratePrintersCallback callback) override;
  void FetchCapabilities(const std::string& device_name,
                         FetchCapabilitiesCallback callback) override;

  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;
  scoped_refptr<PrintBackend> print_backend_;

  DISALLOW_COPY_AND_ASSIGN(PrintBackendHandlerImpl);
};

}  // namespace printing

#endif  // COMPONENTS_PRINTING_SERVICE_PRINT_BACKEND_HANDLER_IMPL_H_
