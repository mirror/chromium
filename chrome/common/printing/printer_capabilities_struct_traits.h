// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_PRINTING_PRINTER_CAPABILITIES_STRUCT_TRAITS_H_
#define CHROME_COMMON_PRINTING_PRINTER_CAPABILITIES_STRUCT_TRAITS_H_

#include "build/build_config.h"
#include "chrome/common/printing/printer_capabilities.mojom.h"
#include "printing/backend/print_backend.h"
#include "printing/print_job_constants.h"

namespace mojo {

template <>
struct EnumTraits<printing::mojom::DuplexMode, printing::DuplexMode> {
  static printing::mojom::DuplexMode ToMojom(printing::DuplexMode mode) {
    switch (mode) {
      case printing::DuplexMode::UNKNOWN_DUPLEX_MODE:
        return printing::mojom::DuplexMode::UNKNOWN_DUPLEX_MODE;
      case printing::DuplexMode::SIMPLEX:
        return printing::mojom::DuplexMode::SIMPLEX;
      case printing::DuplexMode::LONG_EDGE:
        return printing::mojom::DuplexMode::LONG_EDGE;
      case printing::DuplexMode::SHORT_EDGE:
        return printing::mojom::DuplexMode::SHORT_EDGE;
    }
    NOTREACHED() << "Unknown duplex mode " << static_cast<int>(mode);
    return printing::mojom::DuplexMode::UNKNOWN_DUPLEX_MODE;
  }

  static bool FromMojom(printing::mojom::DuplexMode input,
                        printing::DuplexMode* output) {
    switch (input) {
      case printing::mojom::DuplexMode::UNKNOWN_DUPLEX_MODE:
        *output = printing::DuplexMode::UNKNOWN_DUPLEX_MODE;
        return true;
      case printing::mojom::DuplexMode::SIMPLEX:
        *output = printing::DuplexMode::SIMPLEX;
        return true;
      case printing::mojom::DuplexMode::LONG_EDGE:
        *output = printing::DuplexMode::LONG_EDGE;
        return true;
      case printing::mojom::DuplexMode::SHORT_EDGE:
        *output = printing::DuplexMode::SHORT_EDGE;
        return true;
    }
    NOTREACHED() << "Unknown duplex mode " << static_cast<int>(input);
    return false;
  }
};

template <>
struct EnumTraits<printing::mojom::ColorModel, printing::ColorModel> {
  static printing::mojom::ColorModel ToMojom(printing::ColorModel color_model) {
    switch (color_model) {
      case printing::ColorModel::UNKNOWN_COLOR_MODEL:
        return printing::mojom::ColorModel::UNKNOWN_COLOR_MODEL;
      case printing::ColorModel::GRAY:
        return printing::mojom::ColorModel::GRAY;
      case printing::ColorModel::COLOR:
        return printing::mojom::ColorModel::COLOR;
      case printing::ColorModel::CMYK:
        return printing::mojom::ColorModel::CMYK;
      case printing::ColorModel::CMY:
        return printing::mojom::ColorModel::CMY;
      case printing::ColorModel::KCMY:
        return printing::mojom::ColorModel::KCMY;
      case printing::ColorModel::CMY_K:
        return printing::mojom::ColorModel::CMY_K;
      case printing::ColorModel::BLACK:
        return printing::mojom::ColorModel::BLACK;
      case printing::ColorModel::GRAYSCALE:
        return printing::mojom::ColorModel::GRAYSCALE;
      case printing::ColorModel::RGB:
        return printing::mojom::ColorModel::RGB;
      case printing::ColorModel::RGB16:
        return printing::mojom::ColorModel::RGB16;
      case printing::ColorModel::RGBA:
        return printing::mojom::ColorModel::RGBA;
      case printing::ColorModel::COLORMODE_COLOR:
        return printing::mojom::ColorModel::COLORMODE_COLOR;
      case printing::ColorModel::COLORMODE_MONOCHROME:
        return printing::mojom::ColorModel::COLORMODE_MONOCHROME;
      case printing::ColorModel::HP_COLOR_COLOR:
        return printing::mojom::ColorModel::HP_COLOR_COLOR;
      case printing::ColorModel::HP_COLOR_BLACK:
        return printing::mojom::ColorModel::HP_COLOR_BLACK;
      case printing::ColorModel::PRINTOUTMODE_NORMAL:
        return printing::mojom::ColorModel::PRINTOUTMODE_NORMAL;
      case printing::ColorModel::PRINTOUTMODE_NORMAL_GRAY:
        return printing::mojom::ColorModel::PRINTOUTMODE_NORMAL_GRAY;
      case printing::ColorModel::PROCESSCOLORMODEL_CMYK:
        return printing::mojom::ColorModel::PROCESSCOLORMODEL_CMYK;
      case printing::ColorModel::PROCESSCOLORMODEL_GREYSCALE:
        return printing::mojom::ColorModel::PROCESSCOLORMODEL_GREYSCALE;
      case printing::ColorModel::PROCESSCOLORMODEL_RGB:
        return printing::mojom::ColorModel::PROCESSCOLORMODEL_RGB;
      case printing::ColorModel::BROTHER_CUPS_COLOR:
        return printing::mojom::ColorModel::BROTHER_CUPS_COLOR;
      case printing::ColorModel::BROTHER_CUPS_MONO:
        return printing::mojom::ColorModel::BROTHER_CUPS_MONO;
      case printing::ColorModel::BROTHER_BRSCRIPT3_COLOR:
        return printing::mojom::ColorModel::BROTHER_BRSCRIPT3_COLOR;
      case printing::ColorModel::BROTHER_BRSCRIPT3_BLACK:
        return printing::mojom::ColorModel::BROTHER_BRSCRIPT3_BLACK;
    }
    NOTREACHED() << "Unknown color model " << static_cast<int>(color_model);
    return printing::mojom::ColorModel::UNKNOWN_COLOR_MODEL;
  }

  static bool FromMojom(printing::mojom::ColorModel input,
                        printing::ColorModel* output) {
    switch (input) {
      case printing::mojom::ColorModel::UNKNOWN_COLOR_MODEL:
        *output = printing::ColorModel::UNKNOWN_COLOR_MODEL;
        return true;
      case printing::mojom::ColorModel::GRAY:
        *output = printing::ColorModel::GRAY;
        return true;
      case printing::mojom::ColorModel::COLOR:
        *output = printing::ColorModel::COLOR;
        return true;
      case printing::mojom::ColorModel::CMYK:
        *output = printing::ColorModel::CMYK;
        return true;
      case printing::mojom::ColorModel::CMY:
        *output = printing::ColorModel::CMY;
        return true;
      case printing::mojom::ColorModel::KCMY:
        *output = printing::ColorModel::KCMY;
        return true;
      case printing::mojom::ColorModel::CMY_K:
        *output = printing::ColorModel::CMY_K;
        return true;
      case printing::mojom::ColorModel::BLACK:
        *output = printing::ColorModel::BLACK;
        return true;
      case printing::mojom::ColorModel::GRAYSCALE:
        *output = printing::ColorModel::GRAYSCALE;
        return true;
      case printing::mojom::ColorModel::RGB:
        *output = printing::ColorModel::RGB;
        return true;
      case printing::mojom::ColorModel::RGB16:
        *output = printing::ColorModel::RGB16;
        return true;
      case printing::mojom::ColorModel::RGBA:
        *output = printing::ColorModel::RGBA;
        return true;
      case printing::mojom::ColorModel::COLORMODE_COLOR:
        *output = printing::ColorModel::COLORMODE_COLOR;
        return true;
      case printing::mojom::ColorModel::COLORMODE_MONOCHROME:
        *output = printing::ColorModel::COLORMODE_MONOCHROME;
        return true;
      case printing::mojom::ColorModel::HP_COLOR_COLOR:
        *output = printing::ColorModel::HP_COLOR_COLOR;
        return true;
      case printing::mojom::ColorModel::HP_COLOR_BLACK:
        *output = printing::ColorModel::HP_COLOR_BLACK;
        return true;
      case printing::mojom::ColorModel::PRINTOUTMODE_NORMAL:
        *output = printing::ColorModel::PRINTOUTMODE_NORMAL;
        return true;
      case printing::mojom::ColorModel::PRINTOUTMODE_NORMAL_GRAY:
        *output = printing::ColorModel::PRINTOUTMODE_NORMAL_GRAY;
        return true;
      case printing::mojom::ColorModel::PROCESSCOLORMODEL_CMYK:
        *output = printing::ColorModel::PROCESSCOLORMODEL_CMYK;
        return true;
      case printing::mojom::ColorModel::PROCESSCOLORMODEL_GREYSCALE:
        *output = printing::ColorModel::PROCESSCOLORMODEL_GREYSCALE;
        return true;
      case printing::mojom::ColorModel::PROCESSCOLORMODEL_RGB:
        *output = printing::ColorModel::PROCESSCOLORMODEL_RGB;
        return true;
      case printing::mojom::ColorModel::BROTHER_CUPS_COLOR:
        *output = printing::ColorModel::BROTHER_CUPS_COLOR;
        return true;
      case printing::mojom::ColorModel::BROTHER_CUPS_MONO:
        *output = printing::ColorModel::BROTHER_CUPS_MONO;
        return true;
      case printing::mojom::ColorModel::BROTHER_BRSCRIPT3_COLOR:
        *output = printing::ColorModel::BROTHER_BRSCRIPT3_COLOR;
        return true;
      case printing::mojom::ColorModel::BROTHER_BRSCRIPT3_BLACK:
        *output = printing::ColorModel::BROTHER_BRSCRIPT3_BLACK;
        return true;
    }
    NOTREACHED() << "Unknown color model " << static_cast<int>(input);
    return false;
  }
};

template <>
class StructTraits<printing::mojom::PrinterCapsAndDefaultsDataView,
                   printing::PrinterCapsAndDefaults> {
 public:
  static std::string printer_capabilities(
      const printing::PrinterCapsAndDefaults& printer_capabilities) {
    return printer_capabilities.printer_capabilities;
  }
  static std::string caps_mime_type(
      const printing::PrinterCapsAndDefaults& printer_capabilities) {
    return printer_capabilities.caps_mime_type;
  }
  static std::string printer_defaults(
      const printing::PrinterCapsAndDefaults& printer_capabilities) {
    return printer_capabilities.printer_defaults;
  }
  static std::string defaults_mime_type(
      const printing::PrinterCapsAndDefaults& printer_capabilities) {
    return printer_capabilities.defaults_mime_type;
  }

  static bool Read(printing::mojom::PrinterCapsAndDefaultsDataView data,
                   printing::PrinterCapsAndDefaults* out_printer_capabilities);
};

template <>
class StructTraits<printing::mojom::PaperDataView,
                   printing::PrinterSemanticCapsAndDefaults::Paper> {
 public:
  static std::string display_name(
      const printing::PrinterSemanticCapsAndDefaults::Paper& paper) {
    return paper.display_name;
  }
  static std::string vendor_id(
      const printing::PrinterSemanticCapsAndDefaults::Paper& paper) {
    return paper.vendor_id;
  }
  static gfx::Size size_um(
      const printing::PrinterSemanticCapsAndDefaults::Paper& paper) {
    return paper.size_um;
  }

  static bool Read(printing::mojom::PaperDataView paper,
                   printing::PrinterSemanticCapsAndDefaults::Paper* out_paper);
};

template <>
class StructTraits<printing::mojom::PrinterSemanticCapsAndDefaultsDataView,
                   printing::PrinterSemanticCapsAndDefaults> {
 public:
  static bool collate_capable(
      const printing::PrinterSemanticCapsAndDefaults& caps) {
    return caps.collate_capable;
  }
  static bool collate_default(
      const printing::PrinterSemanticCapsAndDefaults& caps) {
    return caps.collate_default;
  }
  static bool copies_capable(
      const printing::PrinterSemanticCapsAndDefaults& caps) {
    return caps.copies_capable;
  }
  static bool duplex_capable(
      const printing::PrinterSemanticCapsAndDefaults& caps) {
    return caps.duplex_capable;
  }
  static printing::DuplexMode duplex_default(
      const printing::PrinterSemanticCapsAndDefaults& caps) {
    return caps.duplex_default;
  }
  static bool color_changeable(
      const printing::PrinterSemanticCapsAndDefaults& caps) {
    return caps.color_changeable;
  }
  static bool color_default(
      const printing::PrinterSemanticCapsAndDefaults& caps) {
    return caps.color_default;
  }
  static printing::ColorModel color_model(
      const printing::PrinterSemanticCapsAndDefaults& caps) {
    return caps.color_model;
  }
  static printing::ColorModel bw_model(
      const printing::PrinterSemanticCapsAndDefaults& caps) {
    return caps.bw_model;
  }
  static std::vector<printing::PrinterSemanticCapsAndDefaults::Paper> papers(
      const printing::PrinterSemanticCapsAndDefaults& caps) {
    return caps.papers;
  }
  static printing::PrinterSemanticCapsAndDefaults::Paper default_paper(
      const printing::PrinterSemanticCapsAndDefaults& caps) {
    return caps.default_paper;
  }
  static std::vector<gfx::Size> dpis(
      const printing::PrinterSemanticCapsAndDefaults& caps) {
    return caps.dpis;
  }
  static gfx::Size default_dpi(
      const printing::PrinterSemanticCapsAndDefaults& caps) {
    return caps.default_dpi;
  }

  static bool Read(printing::mojom::PrinterSemanticCapsAndDefaultsDataView caps,
                   printing::PrinterSemanticCapsAndDefaults* out_caps);
};

}  // namespace mojo

#endif  // CHROME_COMMON_PRINTING_PRINTER_CAPABILITIES_STRUCT_TRAITS_H_
