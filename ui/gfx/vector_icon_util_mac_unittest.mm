// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/vector_icon_util_mac.h"

#include <Cocoa/Cocoa.h>

#include <string>

#include "base/i18n/rtl.h"
#include "base/mac/scoped_nsobject.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/image/image_skia_util_mac.h"
#include "ui/gfx/paint_vector_icon.h"

namespace gfx {

namespace {

NSBitmapImageRep* BitmapImageRepForImage(NSImage* image) {
  [image lockFocus];
  NSBitmapImageRep* rep = [[NSBitmapImageRep alloc]
      initWithFocusedViewRect:NSMakeRect(0, 0, image.size.width,
                                         image.size.height)];
  [image unlockFocus];
  return rep;
}

void VerifySamePixels(NSImage* lhs, NSImage* rhs) {
  base::scoped_nsobject<NSBitmapImageRep> lhs_bitmap(
      BitmapImageRepForImage(lhs));
  base::scoped_nsobject<NSBitmapImageRep> rhs_bitmap(
      BitmapImageRepForImage(rhs));

  NSInteger num_bytes =
      [lhs_bitmap bytesPerPlane] * [lhs_bitmap numberOfPlanes];
  // Assert since test will crash if this is false.
  ASSERT_EQ(num_bytes,
            [rhs_bitmap bytesPerPlane] * [rhs_bitmap numberOfPlanes]);
  EXPECT_GT(num_bytes, 0);

  unsigned char* lhs_pixels = [lhs_bitmap bitmapData];
  unsigned char* rhs_pixels = [rhs_bitmap bitmapData];

  for (NSInteger i = 0; i < num_bytes; i++) {
    EXPECT_EQ(lhs_pixels[i], rhs_pixels[i]);
  }
}

// 20x20 square with origin 0,0
const PathElement elements[] = {
    LINE_TO, 20, 0, LINE_TO, 20, 20, LINE_TO, 0, 20, CLOSE, END,
};
const VectorIcon kSquareIcon = {elements, nullptr};

const char kDefaultRTLLocale[] = "he";  // Hebrew.

}  // namespace

// Test that the image is pixel-identical to rasterizing to ImageSkia
// and converting to NSImage*.
TEST(VectorIconUtilMacTest, SamePixels) {
  SkColor color = SK_ColorBLACK;
  NSImage* image = NSImageWithVectorIcon(kSquareIcon, color);
  NSImage* control_image = NSImageFromImageSkia(gfx::CreateVectorIcon(
      kSquareIcon, gfx::GetDefaultSizeOfVectorIcon(kSquareIcon), color));
  VerifySamePixels(image, control_image);
}

// Test that the icon's default size is used by the two-argument version.
TEST(VectorIconUtilMacTest, DefaultSize) {
  NSImage* image = NSImageWithVectorIcon(kSquareIcon, SK_ColorBLACK);
  CGFloat expected_dimension =
      static_cast<CGFloat>(gfx::GetDefaultSizeOfVectorIcon(kSquareIcon));

  EXPECT_TRUE(NSEqualSizes([image size],
                           NSMakeSize(expected_dimension, expected_dimension)));
}

// Test that the provided size is used in four-argument version.
TEST(VectorIconUtilMacTest, UsesProvidedSize) {
  NSSize size = NSMakeSize(200, 800);
  NSImage* image =
      NSImageWithVectorIcon(kSquareIcon, SK_ColorBLACK, size, false);

  EXPECT_TRUE(NSEqualSizes([image size], size));
}

// Test that when |mirror_in_rtl| is true, the image is pixel-identical to
// rasterizing to ImageSkia and reversing in Cocoa.
TEST(VectorIconUtilMacTest, SamePixelsRTL) {
  // ScopedRTLFlipCanvas checks base::i18n::IsRTL(), so make that true.
  std::string original_locale = base::i18n::GetConfiguredLocale();
  base::i18n::SetICUDefaultLocale(kDefaultRTLLocale);

  NSSize size = NSMakeSize(48, 48);
  SkColor color = SK_ColorBLACK;

  NSImage* image = NSImageWithVectorIcon(kSquareIcon, color, size, true);
  NSImage* control_image = NSImageFromImageSkia(
      gfx::CreateVectorIcon(kSquareIcon, size.width, color));

  // Mirror the control image.
  // From chrome/browser/ui/cocoa/l10n_util.mm by way of Apple's RTL docs
  // goo.gl/cBaFnT
  NSImage* flipped_image = [[[NSImage alloc] initWithSize:size] autorelease];

  [flipped_image lockFocus];
  [[NSGraphicsContext currentContext]
      setImageInterpolation:NSImageInterpolationHigh];

  NSAffineTransform* transform = [NSAffineTransform transform];
  [transform translateXBy:size.width yBy:0];
  [transform scaleXBy:-1 yBy:1];
  [transform concat];

  [control_image drawAtPoint:NSZeroPoint
                    fromRect:NSMakeRect(0, 0, size.width, size.height)
                   operation:NSCompositeSourceOver
                    fraction:1.0];

  [flipped_image unlockFocus];

  VerifySamePixels(image, flipped_image);
  // Restore original locale.
  base::i18n::SetICUDefaultLocale(original_locale);
}

}  // namespace gfx
