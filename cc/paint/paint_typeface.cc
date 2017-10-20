// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/paint_typeface.h"
#include "third_party/skia/include/ports/SkFontConfigInterface.h"
#include "third_party/skia/include/ports/SkFontMgr.h"

namespace cc {

PaintTypeface::PaintTypeface(SkFontID sk_id) : sk_id_(sk_id) {}
PaintTypeface::PaintTypeface(const PaintTypeface& other) = default;
PaintTypeface::PaintTypeface(PaintTypeface&& other) = default;

PaintTypeface& PaintTypeface::operator=(const PaintTypeface& other) = default;
PaintTypeface& PaintTypeface::operator=(PaintTypeface&& other) = default;

sk_sp<SkTypeface> PaintTypeface::CreateTypeface() const {
  switch (type_) {
    case Type::kUninitialized: {
      NOTREACHED();
      return nullptr;
    }
    case Type::kFontConfigInterfaceIdAndTtcIndex: {
      sk_sp<SkFontConfigInterface> fci(SkFontConfigInterface::RefGlobal());
      SkFontConfigInterface::FontIdentity font_identity;
      font_identity.fID = font_config_interface_id_;
      font_identity.fTTCIndex = ttc_index_;
      auto tf = fci->makeTypeface(font_identity);
      return tf;
    }
    case Type::kFilenameAndTtcIndex: {
      return SkTypeface::MakeFromFile(filename_.c_str(), ttc_index_);
    }
    case Type::kFamilyNameAndFontStyle: {
      auto fm(SkFontMgr::RefDefault());
      return fm->legacyMakeTypeface(family_name_.c_str(), font_style_);
    }
    case Type::kWebFont: {
      // TODO(vmpstr): Figure this out.
      return nullptr;
    }
  }
  return nullptr;
}

}  // namespace cc
