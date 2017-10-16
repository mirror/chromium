// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HighContrastImageClassifier_h
#define HighContrastImageClassifier_h

#include <vector>

#include "platform/graphics/GraphicsTypes.h"
#include "platform/graphics/Image.h"

namespace blink {

class PLATFORM_EXPORT HighContrastImageClassifier {
 public:
  HighContrastImageClassifier() = default;
  ~HighContrastImageClassifier() = default;

  // Decides if a high contrast filter should be applied to the image or not.
  bool ShouldApplyHighContrastFilterToImage(Image&);

  bool ComputeImageFeaturesForTesting(Image& image,
                                      std::vector<float>* features) {
    return ComputeImageFeatures(image, features);
  }

 private:
  enum class ColorMode { kColor = 0, kGrayscale = 1 };

  // Computes the features vector for a given image.
  bool ComputeImageFeatures(Image&, std::vector<float>*);

  // Converts image to SkBitmap and returns true if successful.
  bool GetBitmap(Image&, SkBitmap*);

  // Extracts sample pixels, transparency ratio, and background ratio from the
  // given bitmap.
  void GetSamples(const SkBitmap&, std::vector<SkColor>*, float*, float*);

  // Computes the features, given sampled pixels, transparency ratio, and
  // background ratio.
  void GetFeatures(const std::vector<SkColor>&,
                   const float,
                   const float,
                   std::vector<float>*);

  // Makes a decision about image given its features.
  HighContrastClassification ClassifyImage(const std::vector<float>&);

  // Receives sampled pixels and color mode, and returns the ratio of color
  // buckets count to all possible color buckets.
  float ComputeColorBucketsRatio(const std::vector<SkColor>&, const ColorMode);

  // Gets the specified number of samples from a block of the image, and returns
  // the samples and the number of transparent pixels.
  void GetBlockSamples(const SkBitmap&,
                       const int,
                       const int,
                       const int,
                       int,
                       int,
                       std::vector<SkColor>*,
                       int*);

  // Given sampled pixels from a block of image and the number of transparent
  // pixels, decides if a block is part of background or or not.
  bool IsBlockBackground(const std::vector<SkColor>&, const int);
};

}  // namespace blink

#endif  // HighContrastImageClassifier_h
