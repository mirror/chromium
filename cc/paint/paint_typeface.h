// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_PAINT_TYPEFACE_H_
#define CC_PAINT_PAINT_TYPEFACE_H_

#include <vector>

#include "base/logging.h"
#include "base/strings/string16.h"
#include "build/build_config.h"
#include "cc/paint/paint_export.h"
#include "third_party/skia/include/core/SkFontArguments.h"
#include "third_party/skia/include/core/SkTypeface.h"

#if defined(OS_MACOSX)

#include "cc/paint/paint_font_loader_mac.h"

#if __OBJC__
@class NSFont;
#else
class NSFont;
#endif  // __OBJC__

class PaintFontLoaderMac;
#endif  // defined(OS_MACOSX)

namespace cc {

class CC_PAINT_EXPORT PaintTypeface {
 public:
  enum class Type : uint8_t {
    kTestTypeface,  // This should only be used in tests.
    kSkTypeface,
    kFontConfigInterfaceIdAndTtcIndex,
    kFilenameAndTtcIndex,
    kFamilyNameAndFontStyle,
    MAC
  };

  static PaintTypeface TestTypeface();
  static PaintTypeface FromSkTypeface(const sk_sp<SkTypeface>& typeface);
  static PaintTypeface FromFontConfigInterfaceIdAndTtcIndex(int config_id,
                                                            int ttc_index);
  static PaintTypeface FromFilenameAndTtcIndex(const std::string& filename,
                                               int ttc_index);
  static PaintTypeface FromFamilyNameAndFontStyle(
      const std::string& family_name,
      const SkFontStyle& font_style);
  // TODO(vmpstr): Need to add FromWebFont?

#if defined(OS_MACOSX)
  static PaintTypeface FromMAC(NSFont* ns_font,
                               float requested_size,
                               std::vector<SkFontArguments::Axis> axes,
                               const PaintFontLoaderMac& loader);

  static PaintTypeface FromMAC(const base::string16& font_name,
                               float font_size,
                               float requested_size,
                               std::vector<SkFontArguments::Axis> axes,
                               const PaintFontLoaderMac& loader);
#endif

  PaintTypeface();
  PaintTypeface(const PaintTypeface& other);
  PaintTypeface(PaintTypeface&& other);
  ~PaintTypeface();

  PaintTypeface& operator=(const PaintTypeface& other);
  PaintTypeface& operator=(PaintTypeface&& other);
  operator bool() const { return !!sk_typeface_; }

  // This is used when deserialized to force a different typeface id so that it
  // can be matched from SkTextBlob deserialization.
  void SetSkId(SkFontID id) { sk_id_ = id; }

  SkFontID sk_id() const { return sk_id_; }
  const sk_sp<SkTypeface>& ToSkTypeface() const { return sk_typeface_; }

  Type type() const { return type_; }
  int font_config_interface_id() const { return font_config_interface_id_; }
  int ttc_index() const { return ttc_index_; }
  const std::string& filename() const { return filename_; }
  const std::string& family_name() const { return family_name_; }
  const SkFontStyle font_style() const { return font_style_; }

  const base::string16& font_name() const { return font_name_; }
  float font_size() const { return font_size_; }
  float requested_size() const { return requested_size_; }
  const std::vector<SkFontArguments::Axis> axes() const { return axes_; }

 private:
#if defined(OS_MACOSX)
  void CreateSkTypeface(NSFont* ns_font = nullptr,
                        const PaintFontLoaderMac* loader = nullptr);
#else
  void CreateSkTypeface();
#endif

  // This is the font ID that should be used by this PaintTypeface, regardless
  // of the sk_typeface_ on the deserialized end. This value is initialized but
  // can be overridden by SetSkId().
  SkFontID sk_id_;
  sk_sp<SkTypeface> sk_typeface_;
  Type type_ = Type::kSkTypeface;

  int font_config_interface_id_ = 0;
  int ttc_index_ = 0;
  std::string filename_;
  std::string family_name_;
  SkFontStyle font_style_;

  base::string16 font_name_;
  float font_size_ = 0.f;
  float requested_size_ = 0.f;
  std::vector<SkFontArguments::Axis> axes_;
};

}  // namespace cc

#endif  // CC_PAINT_PAINT_TYPEFACE_H_
