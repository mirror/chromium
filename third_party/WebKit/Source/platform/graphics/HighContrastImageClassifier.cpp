// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/HighContrastImageClassifier.h"

#include "third_party/skia/include/utils/SkNullCanvas.h"

namespace {

#define kHashPurgeThreshold 100000
#define kHashPurgePercentage 30

bool IsColorSemiGray(const SkColor& color) {
  return abs(static_cast<int>(SkColorGetR(color)) -
             static_cast<int>(SkColorGetG(color))) +
             abs(static_cast<int>(SkColorGetG(color)) -
                 static_cast<int>(SkColorGetB(color))) <=
         8;
}

}  // namespace

namespace blink {

HighContrastClassification HighContrastImageClassifier::LRUCache::Get(
    const uint32_t key) {
  auto item = hash_map_.find(key);
  if (item == hash_map_.end())
    return HighContrastClassification::kNotClassified;

  // Move current to the end.
  uint32_t last_pos = hash_vector_.size() - 1;
  uint32_t current_pos = item->second;
  if (current_pos != last_pos) {
    item->second = last_pos;
    std::swap(hash_vector_[current_pos], hash_vector_[last_pos]);
    hash_map_.find(hash_vector_[current_pos].first)->second = current_pos;
  }
  return hash_vector_.back().second;
}

void HighContrastImageClassifier::LRUCache::Set(
    const uint32_t key,
    const HighContrastClassification value) {
  DCHECK(hash_map_.find(key) == hash_map_.end());
  hash_vector_.push_back(std::make_pair(key, value));
  hash_map_.insert(std::make_pair(key, hash_vector_.size() - 1));

  // Purge if too big.
  if (hash_map_.size() > purge_threshold_) {
    uint32_t purge_size = purge_threshold_ * purge_percentage_ / 100;
    // Remove hash keys from the map.
    for (size_t i = 0; i < purge_size; i++)
      hash_map_.erase(hash_vector_[i].first);
    // Remove from the vector.
    hash_vector_.erase(hash_vector_.begin(), hash_vector_.begin() + purge_size);
    // Adjust map
    for (auto& item : hash_map_)
      item.second -= purge_size;
  }
}

HighContrastImageClassifier::HighContrastImageClassifier()
    : cached_results_(kHashPurgeThreshold, kHashPurgePercentage) {}

HighContrastImageClassifier::~HighContrastImageClassifier() {}

HighContrastClassification
HighContrastImageClassifier::ShouldApplyHighContrastFilterToImage(
    Image& image) {
  HighContrastClassification result = image.GetHighContrastClassification();
  if (result != HighContrastClassification::kNotClassified)
    return result;

  // TODO: What should we do with other images?
  if (!image.IsBitmapImage())
    return HighContrastClassification::kNegative;

  // Convert image to SkConvas
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
  SkPixmap pixmap(info, pixel_storage.data(), row_bytes);

  // If copy unsuccessful, set result to negative.
  if (!SkImage::MakeFromBitmap(bitmap)->readPixels(pixmap, 0, 0)) {
    result = HighContrastClassification::kNegative;
  } else {
    // Search for cached result.
    uint32_t hash_key = ComputeHashKey(pixmap);
    result = cached_results_.Get(hash_key);
    // If cached result not found, classify it.
    if (result == HighContrastClassification::kNotClassified) {
      result = ClassifyImage(pixmap);
      cached_results_.Set(hash_key, result);
    }
  }

  image.SetHighContrastClassification(result);
  return result;
}

uint32_t HighContrastImageClassifier::ComputeHashKey(const SkPixmap& pixmap) {
  uint32_t hash_key = pixmap.width() + 10000 * pixmap.height();
  const uint32_t* buffer = pixmap.addr32();

  // 611170003 is the largest prime number smaller than (2^32 - 2^24)/7.
  for (int index = pixmap.width() * pixmap.height() - 1; index >= 0; index--)
    hash_key = (hash_key % 611170003) * 7 + (buffer[index] & 0xffffff);
  return hash_key;
}

HighContrastClassification HighContrastImageClassifier::ClassifyImage(
    const SkPixmap& pixmap) {
  std::vector<SkColor> sampled_pixels;
  float transparency_ratio;
  GetSamples(pixmap, &sampled_pixels, &transparency_ratio);

  std::vector<float> features;
  GetFeatures(sampled_pixels, transparency_ratio, &features);

  bool result = (features.size() && features[0] > 0);
  return result ? HighContrastClassification::kPositive
                : HighContrastClassification::kNegative;
}

void HighContrastImageClassifier::GetSamples(
    const SkPixmap& pixmap,
    std::vector<SkColor>* sampled_pixels,
    float* transparency_ratio) {
  const int required_count = 1000;

  int cx = static_cast<int>(ceil(pixmap.width() / sqrt(required_count)));
  int cy = static_cast<int>(ceil(pixmap.height() / sqrt(required_count)));

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

  // Decide if image is monochrome.
  int color_pixels = 0;
  for (const SkColor& sample : sampled_pixels) {
    if (!IsColorSemiGray(sample))
      color_pixels++;
  }
  ColorMode color_mode = (color_pixels > samples_count / 100)
                             ? ColorMode::kColor
                             : ColorMode::kGrayscale;

  features->push_back(color_mode == ColorMode::kColor ? 0 : 1);

#ifndef NDEBUG
  for (auto feature : *features)
    DVLOG(1) << "HCIC-Feature:\t" << feature;
#endif
}

}  // namespace blink
