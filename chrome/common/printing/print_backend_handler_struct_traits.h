// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_PRINTING_PRINT_BACKEND_HANDLER_STRUCT_TRAITS_H_
#define CHROME_COMMON_PRINTING_PRINT_BACKEND_HANDLER_STRUCT_TRAITS_H_

#include <map>
#include <vector>

#include "chrome/common/printing/print_backend_handler.mojom.h"
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

#endif  // CHROME_COMMON_PRINTING_PRINT_BACKEND_HANDLER_STRUCT_TRAITS_H_
