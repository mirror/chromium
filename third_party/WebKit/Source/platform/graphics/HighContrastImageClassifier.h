// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HighContrastImageClassifier_h
#define HighContrastImageClassifier_h

#include <map>
#include <set>

#include "platform/graphics/GraphicsTypes.h"
#include "platform/graphics/Image.h"

namespace blink {

class HighContrastImageClassifier {
 public:
  class LRUCache {
   public:
    LRUCache(uint32_t purge_threshold, uint32_t purge_percentage)
        : purge_threshold_(purge_threshold),
          purge_percentage_(purge_percentage){};

    HighContrastClassification Get(const uint32_t key);
    void Set(const uint32_t key, const HighContrastClassification value);

   private:
    // Key: Hash key, Value: Index in |hash_vector_|
    std::map<uint32_t, uint32_t> hash_map_;
    // First: Hash Key, Second: Value.
    std::vector<std::pair<uint32_t, HighContrastClassification>> hash_vector_;

    // Oldest cached items will be purged when the number of cached items
    // exceeds |purge_threshold_|.
    uint32_t purge_threshold_;

    // |purge_percentage_| percent of cached items will be purged when the size
    // exceeds threshold.
    uint32_t purge_percentage_;
  };

  enum class ColorMode { kColor = 0, kGrayscale = 1 };

  HighContrastImageClassifier();
  ~HighContrastImageClassifier();

  // Decides if a high contrast filter should be applied to the image or not.
  HighContrastClassification ShouldApplyHighContrastFilterToImage(Image&);

 private:
  HighContrastClassification ClassifyImage(const SkPixmap&);

  // Creates cluster centers for color and grayscale images.
  void CreateClusterCenters();
  void CreateClusterCentersForColorMode(const ColorMode&);

  // Two sets of basic clasters for color and grayscale modes.
  struct ClusterSet {
    std::vector<SkColor> centers;
    float normalizer;
    std::map<uint32_t, std::set<int>> mappers;
  } cluster_sets_[2];

  // Finds the closest cluster to the given sample color.
  int FindClosestCluster(const SkColor&, const ClusterSet&);

  // Get sample pixels from the image.
  void GetSamples(const SkPixmap&, std::vector<SkColor>*, float*);

  // Performs a limited clustering on the points and computes futures.
  void GetFeatures(const std::vector<SkColor>&,
                   const float,
                   std::vector<float>*);

  // Computes hash code for a given image's pixels.
  uint32_t ComputeHashKey(const SkPixmap&);

  struct ImageBlock {
    SkIRect rect;
    int width = 0;
    int height = 0;
    int pixels_count = 0;
    std::vector<SkColor> sampled_pixels;
  };

  LRUCache cached_results_;
};

}  // namespace blink

#endif  // HighContrastImageClassifier_h
