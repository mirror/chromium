// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <AppKit/AppKit.h>
#import <AppKit/NSFont.h>
#import <Foundation/Foundation.h>
#import <objc/objc-auto.h>
#import "base/macros.h"
#import "base/strings/sys_string_conversions.h"
#import "build/build_config.h"
#import "cc/paint/paint_font_loader_mac.h"
#import "cc/paint/paint_typeface.h"
#import "third_party/skia/include/core/SkStream.h"
#import "third_party/skia/include/ports/SkFontConfigInterface.h"
#import "third_party/skia/include/ports/SkFontMgr.h"
#import "third_party/skia/include/ports/SkTypeface_mac.h"

namespace cc {

// static
PaintTypeface PaintTypeface::FromMAC(NSFont* ns_font,
                                     float requested_size,
                                     std::vector<SkFontArguments::Axis> axes,
                                     const PaintFontLoaderMac& loader) {
  PaintTypeface typeface;
  typeface.type_ = Type::MAC;
  typeface.font_name_ = base::SysNSStringToUTF16([ns_font fontName]);
  typeface.font_size_ = [ns_font pointSize];
  typeface.requested_size_ = requested_size;
  typeface.axes_ = std::move(axes);
  typeface.CreateSkTypeface(ns_font, &loader);
  return typeface;
}

// static
PaintTypeface PaintTypeface::FromMAC(const base::string16& font_name,
                                     float font_size,
                                     float requested_size,
                                     std::vector<SkFontArguments::Axis> axes,
                                     const PaintFontLoaderMac& loader) {
  PaintTypeface typeface;
  typeface.type_ = Type::MAC;
  typeface.font_name_ = font_name;
  typeface.font_size_ = font_size;
  typeface.requested_size_ = requested_size;
  typeface.axes_ = std::move(axes);
  typeface.CreateSkTypeface(
      [NSFont fontWithName:base::SysUTF16ToNSString(font_name) size:font_size],
      &loader);
  return typeface;
}

void PaintTypeface::CreateSkTypeface(NSFont* ns_font,
                                     const PaintFontLoaderMac* loader) {
  DCHECK(type_ == Type::MAC);
  DCHECK(loader);
  DCHECK(ns_font);

  sk_typeface_ = loader->LoadFont(ns_font, requested_size_);
  if (sk_typeface_ && !axes_.empty()) {
    sk_sp<SkFontMgr> fm(SkFontMgr::RefDefault());
    // TODO(crbug.com/670246): Refactor this to a future Skia API that acccepts
    // axis parameters on system fonts directly.
    sk_typeface_ = fm->makeFromStream(
        sk_typeface_->openStream(nullptr)->duplicate(),
        SkFontArguments().setAxes(axes_.data(), axes_.size()));
  }

  sk_id_ = sk_typeface_ ? sk_typeface_->uniqueID() : 0;
}

}  // namespace cc
