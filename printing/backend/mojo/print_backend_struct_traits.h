// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PRINTING_BACKEND_MOJO_PRINT_BACKEND_STRUCT_TRAITS_H
#define PRINTING_BACKEND_MOJO_PRINT_BACKEND_STRUCT_TRAITS_H

#include <vector>

#include "mojo/public/cpp/bindings/struct_traits.h"
#include "printing/backend/mojo/printing.mojom-shared.h"
#include "printing/backend/print_backend.h"

namespace mojo {

template <>
struct StructTraits<printing::mojom::PrinterBasicInfoDataView,
                    printing::PrinterBasicInfo> {
 public:
  static const std::string& printer_name(
      const printing::PrinterBasicInfo& info) {
    return info.printer_name;
  }

  static const std::string& printer_description(
      const printing::PrinterBasicInfo& info) {
    return info.printer_description;
  }

  static int32_t printer_status(const printing::PrinterBasicInfo& info) {
    return info.printer_status;
  }

  static int32_t is_default(const printing::PrinterBasicInfo& info) {
    return info.is_default;
  }

  static const std::map<std::string, std::string>& options(
      const printing::PrinterBasicInfo& info) {
    return info.options;
  }

  static bool Read(printing::mojom::PrinterBasicInfoDataView data,
                   printing::PrinterBasicInfo* out_info);
};

}  // namespace mojo

#endif  // PRINTING_BACKEND_MOJO_PRINT_BACKEND_STRUCT_TRAITS_H
