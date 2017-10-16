// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/HighContrastImageClassifier.h"

#include "third_party/WebKit/Source/platform/geometry/IntPoint.h"
#include "third_party/skia/include/utils/SkNullCanvas.h"

namespace {

bool IsColorGray(const SkColor& color) {
  return abs(static_cast<int>(SkColorGetR(color)) -
             static_cast<int>(SkColorGetG(color))) +
             abs(static_cast<int>(SkColorGetG(color)) -
                 static_cast<int>(SkColorGetB(color))) <=
         8;
}

void RGBAdd(const SkColor& new_pixel, SkColor4f* sink) {
  sink->fR += SkColorGetR(new_pixel);
  sink->fG += SkColorGetG(new_pixel);
  sink->fB += SkColorGetB(new_pixel);
}

void RGBDivide(SkColor4f* sink, int divider) {
  sink->fR /= divider;
  sink->fG /= divider;
  sink->fB /= divider;
}

float ColorDifference(const SkColor& color1, const SkColor4f& color2) {
  return sqrt((pow(static_cast<float>(SkColorGetR(color1)) - color2.fR, 2.0) +
               pow(static_cast<float>(SkColorGetG(color1)) - color2.fG, 2.0) +
               pow(static_cast<float>(SkColorGetB(color1)) - color2.fB, 2.0)) /
              3.0);
}

const int kPixelsToSample = 1000;

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

// This function computes a single feature vector based on a sample set of image
// pixels. Please refer to |GetSamples| function for description of the sampling
// method, and |GetFeatures| function for description of the features.
bool HighContrastImageClassifier::ComputeImageFeatures(
    Image& image,
    std::vector<float>* features) {
  SkBitmap bitmap;
  if (!GetBitmap(image, &bitmap))
    return false;

  std::vector<SkColor> sampled_pixels;
  float transparency_ratio;
  float background_ratio;
  GetSamples(bitmap, &sampled_pixels, &transparency_ratio, &background_ratio);

  GetFeatures(sampled_pixels, transparency_ratio, background_ratio, features);
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

// The image is separated into 100 blocks, uniformly distributed through the
// width and height of the image. Each block is sampled, and checked if it seems
// to be background or not (See IsBlockBackground for details). Samples from
// none-background blocks are collected, and if they are less than 50% of
// requested samples, the none-background blocks are resampled to get more
// points.
void HighContrastImageClassifier::GetSamples(
    const SkBitmap& bitmap,
    std::vector<SkColor>* sampled_pixels,
    float* transparency_ratio,
    float* background_ratio) {
  int block_width = 1 + bitmap.width() / 10;
  int block_height = 1 + bitmap.height() / 10;
  int pixels_per_block = kPixelsToSample / 100;

  int transparent_pixels = 0;
  int opaque_pixels = 0;
  int blocks_count = 0;

  sampled_pixels->clear();
  std::vector<IntPoint> foreground_blocks;

  for (int y = 0; y < bitmap.height(); y += block_height) {
    for (int x = 0; x < bitmap.width(); x += block_width) {
      std::vector<SkColor> block_samples;
      int block_transparent_pixels;
      GetBlockSamples(bitmap, pixels_per_block, x, y, block_width, block_height,
                      &block_samples, &block_transparent_pixels);
      opaque_pixels += static_cast<int>(block_samples.size());
      transparent_pixels += block_transparent_pixels;

      if (!IsBlockBackground(block_samples, block_transparent_pixels)) {
        sampled_pixels->insert(sampled_pixels->end(), block_samples.begin(),
                               block_samples.end());
        foreground_blocks.push_back(IntPoint(x, y));
      }
      blocks_count++;
    }
  }

  *transparency_ratio = static_cast<float>(transparent_pixels) /
                        (transparent_pixels + opaque_pixels);

  *background_ratio =
      1.0 - static_cast<float>(foreground_blocks.size()) / blocks_count;

  // If acquired samples are less than half of the required, and there are
  // foreground blocks, add more samples from them.
  if (sampled_pixels->size() < kPixelsToSample / 2 &&
      foreground_blocks.size()) {
    pixels_per_block = static_cast<int>(
        ceil((kPixelsToSample - static_cast<float>(sampled_pixels->size())) /
             static_cast<float>(foreground_blocks.size())));
    for (const IntPoint& block : foreground_blocks) {
      std::vector<SkColor> block_samples;
      int temp;
      GetBlockSamples(bitmap, pixels_per_block, block.X(), block.Y(),
                      block_width, block_height, &block_samples, &temp);
      sampled_pixels->insert(sampled_pixels->end(), block_samples.begin(),
                             block_samples.end());
    }
  }
}

// Scans a block of the given bitmap to get the requested number of pixels.
// Retruns the opaque sampled pixels, and the number of transparent sampled
// pixels.
void HighContrastImageClassifier::GetBlockSamples(
    const SkBitmap& bitmap,
    const int required_samples,
    const int block_left,
    const int block_top,
    int block_width,
    int block_height,
    std::vector<SkColor>* sampled_pixels,
    int* transparent_pixels) {
  *transparent_pixels = 0;

  DCHECK(block_left < bitmap.width());
  DCHECK(block_top < bitmap.height());
  if (block_left + block_width > bitmap.width())
    block_width = bitmap.width() - block_left;
  if (block_top + block_height > bitmap.height())
    block_height = bitmap.height() - block_top;

  float step = std::fmax(
      1.0, (block_width * block_height) / static_cast<float>(required_samples));
  int y = block_top;
  float dx = -step / 2.0;
  while (true) {
    dx += step;
    while (dx >= block_width) {
      dx -= block_width;
      y++;
    }
    if (y >= block_top + block_height)
      break;

    DCHECK_LE(block_left + static_cast<int>(dx), bitmap.width());
    SkColor new_sample = bitmap.getColor(block_left + static_cast<int>(dx), y);
    if (SkColorGetA(new_sample) < 128) {
      (*transparent_pixels)++;
      continue;
    }
    sampled_pixels->push_back(new_sample);
  }
}

// Given sampled pixels and transparent pixels count of a block of the image,
// decides if that block is background or not. If more than 80% of the block is
// transparent, or the divergence of colors in the block is less than 5%, the
// block is considered background.
bool HighContrastImageClassifier::IsBlockBackground(
    const std::vector<SkColor>& sampled_pixels,
    const int transparent_pixels) {
  if (static_cast<int>(sampled_pixels.size()) <= transparent_pixels / 4)
    return true;

  SkColor4f average;
  average.fR = 0;
  average.fG = 0;
  average.fB = 0;

  for (const SkColor& sample : sampled_pixels)
    RGBAdd(sample, &average);

  RGBDivide(&average, sampled_pixels.size());

  float divergence = 0;
  for (const auto& sample : sampled_pixels)
    divergence += ColorDifference(sample, average);
  divergence /= sampled_pixels.size() * 128;
  return divergence < 0.05;
}

// This function computes a single feature vector from a sample set of image
// pixels. The current features are:
// 0: 1 if color, 0 if grayscale.
// 1: Ratio of the number of bucketed colors used in the image to all
//    possiblities. Color buckets are represented with 4 bits per color channel.
void HighContrastImageClassifier::GetFeatures(
    const std::vector<SkColor>& sampled_pixels,
    const float transparency_ratio,
    const float background_ratio,
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

  features->resize(2);

  // Feature 0: Is Colorful?
  (*features)[0] = color_mode == ColorMode::kColor;

  // Feature 1: Color Buckets Ratio.
  (*features)[1] = ComputeColorBucketsRatio(sampled_pixels, color_mode);
}

float HighContrastImageClassifier::ComputeColorBucketsRatio(
    const std::vector<SkColor>& sampled_pixels,
    const ColorMode color_mode) {
  std::set<unsigned> buckets;
  // If image is in color, use 4 bits per color channel, otherwise 4 bits for
  // illumination.
  if (color_mode == ColorMode::kColor) {
    for (const SkColor& sample : sampled_pixels) {
      unsigned bucket = ((SkColorGetR(sample) >> 4) << 8) +
                        ((SkColorGetG(sample) >> 4) << 4) +
                        ((SkColorGetB(sample) >> 4));
      buckets.insert(bucket);
    }
  } else {
    for (const SkColor& sample : sampled_pixels) {
      unsigned illumination =
          (SkColorGetR(sample) * 5 + SkColorGetG(sample) * 3 +
           SkColorGetB(sample) * 2) /
          10;
      buckets.insert(illumination / 16);
    }
  }

  // Using 4 bit per channel representation of each color bucket, there would be
  // 2^4 buckets for grayscale images and 2^12 for color images.
  const float max_buckets[] = {16, 4096};
  return static_cast<float>(buckets.size()) /
         max_buckets[color_mode == ColorMode::kColor];
}

HighContrastClassification HighContrastImageClassifier::ClassifyImage(
    const std::vector<float>& features) {
  bool result = false;

  // Shallow decision tree trained by C4.5.
  if (features.size() == 2) {
    float threshold = (features[0] == 0) ? 0.8125 : 0.0166;
    result = features[1] < threshold;
  }

  return result ? HighContrastClassification::kApplyHighContrastFilter
                : HighContrastClassification::kDoNotApplyHighContrastFilter;
}

}  // namespace blink
