// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/printing/printer_capabilities_struct_traits.h"

#include "ui/gfx/geometry/mojo/geometry_struct_traits.h"

namespace mojo {

// static
bool StructTraits<printing::mojom::PrinterCapsAndDefaultsDataView,
                  printing::PrinterCapsAndDefaults>::
    Read(printing::mojom::PrinterCapsAndDefaultsDataView data,
         printing::PrinterCapsAndDefaults* out) {
  return data.ReadPrinterCapabilities(&out->printer_capabilities) &&
         data.ReadCapsMimeType(&out->caps_mime_type) &&
         data.ReadPrinterDefaults(&out->printer_defaults) &&
         data.ReadDefaultsMimeType(&out->defaults_mime_type);
}

// static
bool StructTraits<printing::mojom::PaperDataView,
                  printing::PrinterSemanticCapsAndDefaults::Paper>::
    Read(printing::mojom::PaperDataView data,
         printing::PrinterSemanticCapsAndDefaults::Paper* out) {
  return data.ReadDisplayName(&out->display_name) &&
         data.ReadVendorId(&out->vendor_id) && data.ReadSizeUm(&out->size_um);
}

// static
bool StructTraits<printing::mojom::PrinterSemanticCapsAndDefaultsDataView,
                  printing::PrinterSemanticCapsAndDefaults>::
    Read(printing::mojom::PrinterSemanticCapsAndDefaultsDataView data,
         printing::PrinterSemanticCapsAndDefaults* out) {
  out->collate_capable = data.collate_capable();
  out->collate_default = data.collate_default();

  out->copies_capable = data.copies_capable();

  out->duplex_capable = data.duplex_capable();

  out->color_changeable = data.color_changeable();
  out->color_default = data.color_default();

  return data.ReadDuplexDefault(&out->duplex_default) &&
         data.ReadColorModel(&out->color_model) &&
         data.ReadBwModel(&out->bw_model) && data.ReadPapers(&out->papers) &&
         data.ReadDefaultPaper(&out->default_paper) &&
         data.ReadDpis(&out->dpis) && data.ReadDefaultDpi(&out->default_dpi);
}

}  // namespace mojo
