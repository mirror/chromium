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

const int kOneDimensionalPixelsToSample = 34;

}  // namespace

namespace blink {

bool HighContrastImageClassifier::ShouldApplyHighContrastFilterToImage(
    Image& image) {
  HighContrastClassification result = image.GetHighContrastClassification();
  if (result != HighContrastClassification::kNotClassified)
    return result == HighContrastClassification::kApplyHighContrastFilter;

  std::vector<float> features;
  if (!ComputeImageFeatures(image, &features))
    result = HighContrastClassification::kDoNotApplyHighContrastFilter;
  else
    result = ClassifyImage(features);

  image.SetHighContrastClassification(result);
  return result == HighContrastClassification::kApplyHighContrastFilter;
}

bool HighContrastImageClassifier::ComputeImageFeatures(
    Image& image,
    std::vector<float>* features) {
  SkBitmap bitmap;
  if (!GetBitmap(image, &bitmap))
    return false;

  std::vector<SkColor> sampled_pixels;
  float transparency_ratio;
  GetSamples(bitmap, &sampled_pixels, &transparency_ratio);

  GetFeatures(sampled_pixels, bitmap.width(), bitmap.height(),
              transparency_ratio, features);
  return true;
}

bool HighContrastImageClassifier::GetBitmap(Image& image, SkBitmap* bitmap) {
  if (!image.IsBitmapImage() || !image.width() || !image.height())
    return false;

  bitmap->allocPixels(
      SkImageInfo::MakeN32(image.width(), image.height(), kPremul_SkAlphaType));
  SkCanvas canvas(*bitmap);
  canvas.clear(SK_ColorTRANSPARENT);
  canvas.drawImageRect(image.PaintImageForCurrentFrame().GetSkImage(),
                       SkRect::MakeIWH(image.width(), image.height()), nullptr);
  return true;
}

void HighContrastImageClassifier::GetSamples(
    const SkBitmap& bitmap,
    std::vector<SkColor>* sampled_pixels,
    float* transparency_ratio) {
  int cx = static_cast<int>(
      ceil(bitmap.width() / static_cast<float>(kOneDimensionalPixelsToSample)));
  int cy = static_cast<int>(ceil(
      bitmap.height() / static_cast<float>(kOneDimensionalPixelsToSample)));

  sampled_pixels->clear();
  int transparent_pixels = 0;
  for (int y = 0; y < bitmap.height(); y += cy) {
    for (int x = 0; x < bitmap.width(); x += cx) {
      SkColor new_sample = bitmap.getColor(x, y);
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
    const int width,
    const int height,
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

  features->resize(6);

  // Feature 0: Min(W,H)
  (*features)[0] = std::min(width, height);

  // Feature 1: Min(W,H)/ Max(W,H)
  (*features)[1] = (*features)[0] / std::max(width, height);

  // Feature 2: Transparency Ratio
  (*features)[2] = transparency_ratio;

  // Feature 3: Color Buckets Ratio
  (*features)[3] = CountColorBuckets(sampled_pixels) /
                   (color_mode == ColorMode::kColor ? 4096.0 : 16.0);

  // Feature 4: Color Buckets Ratio / Image Size.
  (*features)[4] =
      CountColorBuckets(sampled_pixels) / static_cast<float>(width * height);

  // Feature 5: Is Colorful?
  (*features)[5] = color_mode == ColorMode::kColor;
}

int HighContrastImageClassifier::CountColorBuckets(
    const std::vector<SkColor>& sampled_pixels) {
  std::set<unsigned> buckets;
  for (const SkColor& sample : sampled_pixels) {
    unsigned bucket = ((SkColorGetR(sample) >> 4) << 8) +
                      ((SkColorGetG(sample) >> 4) << 4) +
                      ((SkColorGetB(sample) >> 4));
    buckets.insert(bucket);
  }

  return static_cast<int>(buckets.size());
}

HighContrastClassification HighContrastImageClassifier::ClassifyImage(
    const std::vector<float>& features) {
  bool result = (features.size() && features[0] < 1);
  return result ? HighContrastClassification::kApplyHighContrastFilter
                : HighContrastClassification::kDoNotApplyHighContrastFilter;
}

}  // namespace blink
