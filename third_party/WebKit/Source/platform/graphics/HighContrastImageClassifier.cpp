// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/HighContrastImageClassifier.h"

#include "third_party/skia/include/utils/SkNullCanvas.h"

namespace {

bool IsColorGray(const SkColor& color) {
  return abs(static_cast<int>(SkColorGetR(color)) -
             static_cast<int>(SkColorGetG(color))) +
             abs(static_cast<int>(SkColorGetG(color)) -
                 static_cast<int>(SkColorGetB(color))) <=
         8;
}

enum class ColorMode { kColor = 0, kGrayscale = 1 };

const int kPixelsToSample = 1000;

}  // namespace

namespace blink {

bool HighContrastImageClassifier::ShouldApplyHighContrastFilterToImage(
    Image& image) {
  HighContrastClassification result = image.GetHighContrastClassification();
  if (result != HighContrastClassification::kNotClassified)
    return result == HighContrastClassification::kApplyHighContrastFilter;

  SkPixmap pixmap;
  if (!image.IsBitmapImage() || !GetPixels(image, &pixmap)) {
    result = HighContrastClassification::kDoNotApplyHighContrastFilter;
  } else {
    std::vector<SkColor> sampled_pixels;
    float transparency_ratio;
    GetSamples(pixmap, &sampled_pixels, &transparency_ratio);

    std::vector<float> features;
    GetFeatures(sampled_pixels, transparency_ratio, &features);

    result = ClassifyImage(features);
  }

  image.SetHighContrastClassification(result);
  return result == HighContrastClassification::kApplyHighContrastFilter;
}

bool HighContrastImageClassifier::GetPixels(Image& image, SkPixmap* pixels) {
  SkBitmap bitmap;
  bitmap.allocPixels(
      SkImageInfo::MakeN32(image.width(), image.height(), kPremul_SkAlphaType));
  SkCanvas canvas(bitmap);
  canvas.clear(SK_ColorTRANSPARENT);
  canvas.drawImageRect(image.PaintImageForCurrentFrame().GetSkImage(),
                       SkRect::MakeIWH(image.width(), image.height()), nullptr);

  // Copy the bits into a buffer in RGBA_8888 unpremultiplied format.
  SkImageInfo info =
      SkImageInfo::Make(image.width(), image.height(), kRGBA_8888_SkColorType,
                        kUnpremul_SkAlphaType);
  size_t row_bytes = info.minRowBytes();
  Vector<char> pixel_storage(info.getSafeSize(row_bytes));
  pixels->reset(info, pixel_storage.data(), row_bytes);

  return SkImage::MakeFromBitmap(bitmap)->readPixels(*pixels, 0, 0);
}

void HighContrastImageClassifier::GetSamples(
    const SkPixmap& pixmap,
    std::vector<SkColor>* sampled_pixels,
    float* transparency_ratio) {
  int cx = static_cast<int>(ceil(pixmap.width() / sqrt(kPixelsToSample)));
  int cy = static_cast<int>(ceil(pixmap.height() / sqrt(kPixelsToSample)));

  sampled_pixels->clear();
  int transparent_pixels = 0;
  for (int y = 0; y < pixmap.height(); y += cy) {
    for (int x = 0; x < pixmap.width(); x += cx) {
      SkColor new_sample = pixmap.getColor(x, y);
      if (SkColorGetA(new_sample) < 128)
        transparent_pixels++;
      else
        sampled_pixels->push_back(new_sample);
    }
  }

  *transparency_ratio = (transparent_pixels * 1.0) /
                        (transparent_pixels + sampled_pixels->size());
}

void HighContrastImageClassifier::GetFeatures(
    const std::vector<SkColor>& sampled_pixels,
    const float transparency_ratio,
    std::vector<float>* features) {
  int samples_count = static_cast<int>(sampled_pixels.size());

  // Is image grayscale.
  int color_pixels = 0;
  for (const SkColor& sample : sampled_pixels) {
    if (!IsColorGray(sample))
      color_pixels++;
  }
  ColorMode color_mode = (color_pixels > samples_count / 100)
                             ? ColorMode::kColor
                             : ColorMode::kGrayscale;

  features->push_back(color_mode == ColorMode::kColor ? 1 : 0);
}

HighContrastClassification HighContrastImageClassifier::ClassifyImage(
    const std::vector<float>& features) {
  bool result = (features.size() && features[0] < 1);
  return result ? HighContrastClassification::kApplyHighContrastFilter
                : HighContrastClassification::kDoNotApplyHighContrastFilter;
}

}  // namespace blink
