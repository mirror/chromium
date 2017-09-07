// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/HighContrastImageClassifier.h"

#include "third_party/skia/include/utils/SkNullCanvas.h"

namespace blink {

#define kHashPurgeThreshold 100000
#define kHashPurgePercentage 30

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

// Computes difference between two colors
float ColorDifference(const SkColor& color1, const SkColor& color2) {
  return sqrt((pow(static_cast<float>(SkColorGetR(color1)) -
                       static_cast<float>(SkColorGetR(color2)),
                   2.0) +
               pow(static_cast<float>(SkColorGetG(color1)) -
                       static_cast<float>(SkColorGetG(color2)),
                   2.0) +
               pow(static_cast<float>(SkColorGetB(color1)) -
                       static_cast<float>(SkColorGetB(color2)),
                   2.0)) /
              3.0);
}

float ColorDifference(const SkColor& color1, const SkColor4f& color2) {
  return sqrt((pow(static_cast<float>(SkColorGetR(color1)) - color2.fR, 2.0) +
               pow(static_cast<float>(SkColorGetG(color1)) - color2.fG, 2.0) +
               pow(static_cast<float>(SkColorGetB(color1)) - color2.fB, 2.0)) /
              3.0);
}

HighContrastImageClassifier::HighContrastImageClassifier()
    : cached_results_(kHashPurgeThreshold, kHashPurgePercentage) {
  CreateClusterCenters();
}

HighContrastImageClassifier::~HighContrastImageClassifier() {}

void HighContrastImageClassifier::CreateClusterCenters() {
  CreateClusterCentersForColorMode(ColorMode::kColor);
  CreateClusterCentersForColorMode(ColorMode::kGrayscale);
}

void HighContrastImageClassifier::CreateClusterCentersForColorMode(
    const HighContrastImageClassifier::ColorMode& color_mode) {
  ClusterSet& cluster_set = cluster_sets_[static_cast<int>(color_mode)];
  int cluster_step;

  if (color_mode == ColorMode::kColor) {
    cluster_step = 32;
    for (uint32_t r = cluster_step / 2; r < 256; r += cluster_step) {
      for (uint32_t g = cluster_step / 2; g < 256; g += cluster_step) {
        for (uint32_t b = cluster_step / 2; b < 256; b += cluster_step) {
          cluster_set.centers.push_back(SkColorSetARGBInline(0xff, r, g, b));
        }
      }
    }
  } else {
    cluster_step = 16;
    for (uint32_t g = cluster_step / 2; g < 256; g += cluster_step) {
      cluster_set.centers.push_back(SkColorSetARGBInline(0xff, g, g, g));
    }
  }

  cluster_set.normalizer =
      ColorDifference(SkColorSetARGBInline(0xff, 0, 0, 0),
                      SkColorSetARGBInline(0xff, cluster_step / 2,
                                           cluster_step / 2, cluster_step / 2));

  // Find Fast Mappers, slowly!
  for (uint32_t r = 0; r < 256; r += 4) {
    for (uint32_t g = 0; g < 256; g += 4) {
      for (uint32_t b = 0; b < 256; b += 4) {
        SkColor sample = SkColorSetARGBInline(0xff, r, g, b);
        int closest = 0;
        float least_difference = 256;

        for (int cluster = 0;
             cluster < static_cast<int>(cluster_set.centers.size());
             cluster++) {
          float difference =
              ColorDifference(cluster_set.centers[cluster], sample);
          if (difference < least_difference) {
            least_difference = difference;
            closest = cluster;
          }
        }

        uint32_t key = ((b >> 4) << 8) + (g & 0xf0) + (r >> 4);
        auto slot = cluster_set.mappers.find(key);
        if (slot == cluster_set.mappers.end()) {
          std::set<int> new_set = {closest};
          cluster_set.mappers.insert(std::make_pair(key, new_set));
        } else {
          slot->second.insert(closest);
        }
      }
    }
  }
}

int HighContrastImageClassifier::FindClosestCluster(
    const SkColor& sample,
    const ClusterSet& cluster_set) {
  uint32_t key = ((SkColorGetB(sample) >> 4) << 8) +
                 (SkColorGetG(sample) & 0xf0) + (SkColorGetR(sample) >> 4);
  int closest = 0;
  float least_difference = 256;

  for (auto cluster : cluster_set.mappers.at(key)) {
    float difference = ColorDifference(cluster_set.centers[cluster], sample);
    if (difference < least_difference) {
      least_difference = difference;
      closest = cluster;
    }
  }
  return closest;
}

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

  // Simple classification based on one feature is done until neural network is
  // added.
  bool result = (features.size() > 3) && (features[3] < 0.1);
#ifndef NDEBUG
  LOG(INFO) << "CLASSIFIED " << result << " as feature[3] is " << features[3];
#endif
  return result ? HighContrastClassification::kPositive
                : HighContrastClassification::kNegative;
}

void HighContrastImageClassifier::GetSamples(
    const SkPixmap& pixmap,
    std::vector<SkColor>* sampled_pixels,
    float* transparency_ratio) {
  int required_count = 1000;

  // Separate image to a number of blocks. In each block, select a set of
  // samples, compute divergence of samples, decide if block is background or
  // not.
  int block_size_width = 1 + pixmap.width() / 10;
  int block_size_height = 1 + pixmap.height() / 10;

  // Create Blocks.
  std::vector<ImageBlock> blocks;
  int total_size = 0;
  for (int y = 0; y < pixmap.height(); y += block_size_height) {
    for (int x = 0; x < pixmap.width(); x += block_size_width) {
      ImageBlock block;
      block.rect.set(x, y, std::min(x + block_size_width, pixmap.width()),
                     std::min(y + block_size_height, pixmap.height()));

      block.pixels_count = block.rect.width() * block.rect.height();
      blocks.push_back(block);
      total_size += block.pixels_count;
    }
  }

  // Get Samples and compute divergence. Drop samples if divergence too low.
  int total_samples = 0;
  int active_blocks_size = 0;
  int transparent_pixels = 0;
  int opaque_pixels = 0;
  for (auto& block : blocks) {
    SkColor4f block_average;
    block_average.fR = 0;
    block_average.fG = 0;
    block_average.fB = 0;
    float d = sqrt(required_count * block.pixels_count / (1.0 * total_size));
    int dx = std::max(1, static_cast<int>(block.rect.width() / d));
    int dy = std::max(1, static_cast<int>(block.rect.height() / d));
    for (int y = block.rect.fTop; y < block.rect.fBottom; y += dy) {
      for (int x = block.rect.fLeft; x < block.rect.fRight; x += dx) {
        SkColor new_sample = pixmap.getColor(x, y);
        if (SkColorGetA(new_sample) < 128) {
          transparent_pixels++;
          continue;
        }
        opaque_pixels++;
        block.sampled_pixels.push_back(new_sample);
        RGBAdd(new_sample, &block_average);
      }
    }
    if (block.sampled_pixels.size()) {
      int block_size = static_cast<int>(block.sampled_pixels.size());
      RGBDivide(&block_average, block_size);

      // Compute Divergence
      float divergence = 0;
      for (const auto& color : block.sampled_pixels)
        divergence += ColorDifference(color, block_average);
      divergence /= block_size * 128;

      // Drop?
      if (divergence < 0.05) {
        block.sampled_pixels.clear();
      } else {
        total_samples += static_cast<int>(block.sampled_pixels.size());
        active_blocks_size += static_cast<int>(block.pixels_count);
      }
    }
  }

  *transparency_ratio =
      (transparent_pixels * 1.0) / (transparent_pixels + opaque_pixels);

  if (active_blocks_size) {
    // Get new samples if required.
    int remaining_samples = std::max(0, required_count - total_samples);

    for (auto& block : blocks) {
      if (block.sampled_pixels.size()) {
        sampled_pixels->insert(sampled_pixels->end(),
                               block.sampled_pixels.begin(),
                               block.sampled_pixels.end());
        // Add extra samples.
        int extra =
            (remaining_samples * block.pixels_count) / active_blocks_size;
        while (extra--) {
          int x = block.rect.fLeft + rand() % block.rect.width();
          int y = block.rect.fTop + rand() % block.rect.height();
          SkColor new_sample = pixmap.getColor(x, y);
          if (SkColorGetA(new_sample) >= 128)
            sampled_pixels->push_back(new_sample);
        }
      }
    }
  } else {
    // It's just one solid background. add 1 pixel.
    sampled_pixels->push_back(pixmap.getColor(0, 0));
  }
}

void HighContrastImageClassifier::GetFeatures(
    const std::vector<SkColor>& sampled_pixels,
    const float transparency_ratio,
    std::vector<float>* features) {
  int samples_count = static_cast<int>(sampled_pixels.size());

  // Decide if image is monochrome.
  int color_pixels = 0;
  for (const SkColor& sample : sampled_pixels) {
    if (abs(static_cast<int>(SkColorGetR(sample)) -
            static_cast<int>(SkColorGetG(sample))) +
            abs(static_cast<int>(SkColorGetG(sample)) -
                static_cast<int>(SkColorGetB(sample))) >
        8)
      color_pixels++;
  }
  ColorMode color_mode = (color_pixels > samples_count / 100)
                             ? ColorMode::kColor
                             : ColorMode::kGrayscale;
  const ClusterSet& cluster_set = cluster_sets_[static_cast<int>(color_mode)];
  int clusters_count = static_cast<int>(cluster_set.centers.size());

  // Compute cluster assignments and recompute centers.
  SkColor4f black_color;
  black_color.fR = 0;
  black_color.fG = 0;
  black_color.fB = 0;
  std::vector<SkColor4f> new_cluster_centers(clusters_count, black_color);
  std::vector<int> new_cluster_sizes(clusters_count);
  std::vector<int> assigned_clusters;

  for (int sample = 0; sample < samples_count; sample++) {
    int closest = FindClosestCluster(sampled_pixels[sample], cluster_set);
    // Assign, add.
    assigned_clusters.push_back(closest);
    new_cluster_sizes[closest]++;
    RGBAdd(sampled_pixels[sample], &new_cluster_centers[closest]);
  }

  // Recompute centers
  for (int cluster = 0; cluster < clusters_count; cluster++) {
    if (new_cluster_sizes[cluster])
      RGBDivide(&new_cluster_centers[cluster], new_cluster_sizes[cluster]);
  }

  // Compute divergences.
  std::vector<float> divergences(clusters_count, 0);
  for (int sample = 0; sample < samples_count; sample++) {
    divergences[assigned_clusters[sample]] += ColorDifference(
        sampled_pixels[sample], new_cluster_centers[assigned_clusters[sample]]);
  }

  // Compute features
  int none_empties[3] = {0, 0, 0};
  float divergences_sum[3] = {0, 0, 0};
  float divergences_average[3] = {0, 0, 0};

  // Separate Background
  int background = 0;
  for (int cluster = 0; cluster < clusters_count; cluster++) {
    if (new_cluster_sizes[cluster] >= samples_count / 2) {
      background += new_cluster_sizes[cluster];
      new_cluster_sizes[cluster] = 0;
    }
  }
  float background_ratio =
      std::max((2.0 * background) / samples_count - 1.0, 0.0);
  samples_count -= background;

  for (int cluster = 0; cluster < clusters_count; cluster++) {
    if (new_cluster_sizes[cluster]) {
      int cluster_class =
          (new_cluster_sizes[cluster] > samples_count / 10) ? 1 : 2;

      none_empties[0]++;
      none_empties[cluster_class]++;

      float divergence = divergences[cluster] /
                         (new_cluster_sizes[cluster] * cluster_set.normalizer);

      divergences_sum[0] += divergence * new_cluster_sizes[cluster];
      divergences_sum[cluster_class] += divergence * new_cluster_sizes[cluster];

      divergences_average[0] += divergence;
      divergences_average[cluster_class] += divergence;

      DVLOG(1) << "HCIC-Cluster:\t"
               << "(" << static_cast<int>(new_cluster_centers[cluster].fR)
               << "," << static_cast<int>(new_cluster_centers[cluster].fG)
               << "," << static_cast<int>(new_cluster_centers[cluster].fB)
               << ")\t" << divergence << ",\t"
               << divergence * new_cluster_sizes[cluster] << ",\t"
               << cluster_class;
    }
  }

  DVLOG(1) << "BG:\t" << background_ratio;
  DVLOG(1) << "NORMALIZER:\t" << cluster_set.normalizer;

  if (!samples_count)
    samples_count++;
  if (!clusters_count)
    clusters_count++;

  features->push_back(color_mode == ColorMode::kColor ? 1 : 0);
  features->push_back(transparency_ratio);
  features->push_back(background_ratio);
  for (int i = 0; i < 3; i++) {
    features->push_back(none_empties[i] * 1.0 / clusters_count);
    features->push_back(divergences_sum[i] / samples_count);
    features->push_back(divergences_average[i] / std::max(1, none_empties[i]));
  }

  for (auto feature : *features)
    DVLOG(1) << "HCIC-Feature:\t" << feature;
}

}  // namespace blink
