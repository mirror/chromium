// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_ADDRESS_TRANSLATOR_H_
#define CHROME_INSTALLER_ZUCCHINI_ADDRESS_TRANSLATOR_H_

#include <memory>

#include "chrome/installer/zucchini/image_utils.h"

namespace zucchini {

// Virtual Address relative to some base address (RVA).
using rva_t = uint32_t;

// Following interfaces are used to convert between offsets and RVAs. Note that
// offsets don't always have to be contained in the image file, since some RVAs
// can't be map to such offset, as long as the conversion is reversible.

// Interface for converting a file offset to an RVA.
class RVAToOffsetTranslator {
 public:
  virtual ~RVAToOffsetTranslator() = default;

  virtual offset_t Apply(rva_t rva) = 0;
};

// Interface for converting an rva to a file offset.
class OffsetToRVATranslator {
 public:
  virtual ~OffsetToRVATranslator() = default;

  virtual rva_t Apply(offset_t offset) = 0;
};

// Interface for creating translators of both direction.
class AddressTranslatorFactory {
 public:
  virtual ~AddressTranslatorFactory() = default;

  virtual std::unique_ptr<RVAToOffsetTranslator> MakeRVAToOffsetTranslator()
      const = 0;

  virtual std::unique_ptr<OffsetToRVATranslator> MakeOffsetToRVATranslator()
      const = 0;
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_ADDRESS_TRANSLATOR_H_
