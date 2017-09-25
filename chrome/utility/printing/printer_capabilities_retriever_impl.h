// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_UTILITY_PRINTING_PRINTER_CAPABILITIES_RETRIEVER_IMPL_H_
#define CHROME_UTILITY_PRINTING_PRINTER_CAPABILITIES_RETRIEVER_IMPL_H_

#include <memory>

#include "base/macros.h"
#include "chrome/common/printing/printer_capabilities.mojom.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace printing {

class PrinterCapabilitiesRetrieverImpl
    : public printing::mojom::PrinterCapabilitiesRetriever {
 public:
  static void Create(mojom::PrinterCapabilitiesRetrieverRequest request);

  PrinterCapabilitiesRetrieverImpl();
  ~PrinterCapabilitiesRetrieverImpl() override;

 private:
  void GetPrinterCapsAndDefaults(
      const std::string& printer_name,
      GetPrinterCapsAndDefaultsCallback callback) override;

  void GetPrinterSemanticCapsAndDefaults(
      const std::string& printer_name,
      GetPrinterSemanticCapsAndDefaultsCallback callback) override;

  DISALLOW_COPY_AND_ASSIGN(PrinterCapabilitiesRetrieverImpl);
};

}  // namespace printing

#endif  // CHROME_UTILITY_PRINTING_PRINTER_CAPABILITIES_RETRIEVER_IMPL_H_
