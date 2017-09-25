// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <vector>

#include "chrome/common/printing/print_backend_handler_struct_traits.h"

namespace mojo {
bool StructTraits<printing::mojom::PrinterBasicInfoDataView,
                  printing::PrinterBasicInfo>::
    Read(printing::mojom::PrinterBasicInfoDataView data,
         printing::PrinterBasicInfo* out_info) {
  if (!data.ReadPrinterName(&(out_info->printer_name)) ||
      out_info->printer_name.empty())
    return false;

  if (!data.ReadPrinterDescription(&out_info->printer_description))
    return false;

  out_info->printer_status = data.printer_status();
  out_info->is_default = data.is_default();
  if (!data.ReadOptions(&out_info->options))
    return false;

  return true;
}
}  // namespace mojo
