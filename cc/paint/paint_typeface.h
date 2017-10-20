// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_PAINT_TYPEFACE_H_
#define CC_PAINT_PAINT_TYPEFACE_H_

#include "base/logging.h"
#include "cc/paint/paint_export.h"
#include "third_party/skia/include/core/SkTypeface.h"

namespace cc {

class CC_PAINT_EXPORT PaintTypeface {
 public:
  enum class Type : uint8_t {
    kUninitialized,
    kFontConfigInterfaceIdAndTtcIndex,
    kFilenameAndTtcIndex,
    kFamilyNameAndFontStyle,
    kWebFont
  };

  explicit PaintTypeface(SkFontID sk_id = 0);
  PaintTypeface(const PaintTypeface& other);
  PaintTypeface(PaintTypeface&& other);

  PaintTypeface& operator=(const PaintTypeface& other);
  PaintTypeface& operator=(PaintTypeface&& other);

  void AssertInitialized() const { DCHECK_NE(type_, Type::kUninitialized); }

  SkFontID sk_id() const { return sk_id_; }
  Type type() const { return type_; }
  int font_config_interface_id() const { return font_config_interface_id_; }
  int ttc_index() const { return ttc_index_; }
  const std::string& filename() const { return filename_; }
  const std::string& family_name() const { return family_name_; }
  const SkFontStyle font_style() const { return font_style_; }

  void SetFontConfigInterfaceIdAndTtcIndex(int config_id, int ttc_index) {
    DCHECK_EQ(type_, Type::kUninitialized);
    type_ = Type::kFontConfigInterfaceIdAndTtcIndex;
    font_config_interface_id_ = config_id;
    ttc_index_ = ttc_index;
  }

  void SetFilenameAndTtcIndex(const std::string& filename, int ttc_index) {
    DCHECK_EQ(type_, Type::kUninitialized);
    type_ = Type::kFilenameAndTtcIndex;
    filename_ = filename;
    ttc_index_ = ttc_index;
  }

  void SetFamilyNameAndFontStyle(const std::string& family_name,
                                 const SkFontStyle& font_style) {
    DCHECK_EQ(type_, Type::kUninitialized);
    type_ = Type::kFamilyNameAndFontStyle;
    family_name_ = family_name;
    font_style_ = font_style;
  }

  void SetWebFont() {
    DCHECK_EQ(type_, Type::kUninitialized);
    type_ = Type::kWebFont;
  }

  sk_sp<SkTypeface> CreateTypeface() const;

 private:
  SkFontID sk_id_ = 0;
  Type type_ = Type::kUninitialized;
  int font_config_interface_id_ = 0;
  int ttc_index_ = 0;
  std::string filename_;
  std::string family_name_;
  SkFontStyle font_style_;
};

}  // namespace cc

#endif  // CC_PAINT_PAINT_TYPEFACE_H_
