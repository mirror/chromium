// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/utility/printing/printer_capabilities_retriever_impl.h"

#include "chrome/common/crash_keys.h"
#include "printing/backend/print_backend.h"

namespace printing {

// static
void PrinterCapabilitiesRetrieverImpl::Create(
    mojom::PrinterCapabilitiesRetrieverRequest request) {
  mojo::MakeStrongBinding(base::MakeUnique<PrinterCapabilitiesRetrieverImpl>(),
                          std::move(request));
}

PrinterCapabilitiesRetrieverImpl::PrinterCapabilitiesRetrieverImpl() = default;

PrinterCapabilitiesRetrieverImpl::~PrinterCapabilitiesRetrieverImpl() = default;

void PrinterCapabilitiesRetrieverImpl::GetPrinterCapsAndDefaults(
    const std::string& printer_name,
    GetPrinterCapsAndDefaultsCallback callback) {
  scoped_refptr<PrintBackend> print_backend =
      PrintBackend::CreateInstance(nullptr);
  PrinterCapsAndDefaults printer_info;

  crash_keys::ScopedPrinterInfo crash_key(
      print_backend->GetPrinterDriverInfo(printer_name));

  bool success =
      print_backend->GetPrinterCapsAndDefaults(printer_name, &printer_info);
  std::move(callback).Run(success, printer_name, printer_info);
}

void PrinterCapabilitiesRetrieverImpl::GetPrinterSemanticCapsAndDefaults(
    const std::string& printer_name,
    GetPrinterSemanticCapsAndDefaultsCallback callback) {
  scoped_refptr<PrintBackend> print_backend =
      PrintBackend::CreateInstance(nullptr);
  PrinterSemanticCapsAndDefaults printer_info;

  crash_keys::ScopedPrinterInfo crash_key(
      print_backend->GetPrinterDriverInfo(printer_name));

  bool success = print_backend->GetPrinterSemanticCapsAndDefaults(
      printer_name, &printer_info);
  std::move(callback).Run(success, printer_name, printer_info);
}

}  // namespace printing
