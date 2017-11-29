// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/HighContrastImageClassifier.h"

#include "platform/SharedBuffer.h"
#include "platform/graphics/BitmapImage.h"
#include "platform/testing/TestingPlatformSupportWithMockScheduler.h"
#include "platform/testing/UnitTestHelpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
const float kEpsilon = 0.00001;

}  // namespace

namespace blink {

class HighContrastImageClassifierTest : public ::testing::Test {
 public:
  // Loads the image from |file_name|, computes features vector into |features|,
  // and returns the classification result.
  bool GetFeaturesAndClassification(const std::string& file_name,
                                    std::vector<float>* features) {
    SCOPED_TRACE(file_name);
    scoped_refptr<BitmapImage> image = LoadImage(file_name);
    classifier_.SetRandomGeneratorForTesting();
    classifier_.ComputeImageFeaturesForTesting(*image.get(), features);
    return classifier_.ShouldApplyHighContrastFilterToImage(*image.get());
  }

  void AssertFeaturesEqual(const std::vector<float>& features,
                           const std::vector<float>& expected_features) {
    EXPECT_EQ(features.size(), expected_features.size());
    for (unsigned i = 0; i < features.size(); i++) {
      EXPECT_NEAR(features[i], expected_features[i], kEpsilon)
          << "Feature " << i;
    }
  }

  HighContrastImageClassifier* classifier() { return &classifier_; }

 protected:
  scoped_refptr<BitmapImage> LoadImage(const std::string& file_name) {
    String file_path = testing::BlinkRootDir();
    file_path.append(file_name.c_str());
    scoped_refptr<SharedBuffer> image_data = testing::ReadFromFile(file_path);
    EXPECT_TRUE(image_data.get() && image_data.get()->size());

    scoped_refptr<BitmapImage> image = BitmapImage::Create();
    image->SetData(image_data, true);
    return image;
  }

  ScopedTestingPlatformSupport<TestingPlatformSupportWithMockScheduler>
      platform_;
  HighContrastImageClassifier classifier_;
};

TEST_F(HighContrastImageClassifierTest, FeaturesAndClassification) {
  std::vector<float> features;

  // Test Case 1:
  // Grayscale
  // Color Buckets Ratio: Low
  // Decision Tree: Apply
  // Neural Network: NA
  EXPECT_TRUE(GetFeaturesAndClassification(
      "/LayoutTests/images/resources/grid-large.png", &features));
  EXPECT_EQ(classifier()->ClassifyImageUsingDecisionTreeForTesting(features),
            HighContrastClassification::kApplyHighContrastFilter);
  AssertFeaturesEqual(features, {0.0f, 0.25f, 0.0f, 0.1f});

  // Test Case 2:
  // Grayscale
  // Color Buckets Ratio: Medium
  // Decision Tree: Can't Decide
  // Neural Network: Apply
  EXPECT_TRUE(GetFeaturesAndClassification(
      "/LayoutTests/images/resources/apng08-ref.png", &features));
  EXPECT_EQ(classifier()->ClassifyImageUsingDecisionTreeForTesting(features),
            HighContrastClassification::kNotClassified);
  AssertFeaturesEqual(features, {0.0f, 0.875f, 0.409f, 0.59f});

  // Test Case 3:
  // Color
  // Color Buckets Ratio: Low
  // Decision Tree: Apply
  // Neural Network: N/A
  EXPECT_TRUE(GetFeaturesAndClassification(
      "/LayoutTests/images/resources/apng11-ref.png", &features));
  EXPECT_EQ(classifier()->ClassifyImageUsingDecisionTreeForTesting(features),
            HighContrastClassification::kApplyHighContrastFilter);
  AssertFeaturesEqual(features, {1.0f, 0.003906f, 0.0f, 0.42f});

  // Test Case 4:
  // Color
  // Color Buckets Ratio: Medium
  // Decision Tree: Can't Decide
  // Neural Network: Don't Apply
  EXPECT_FALSE(GetFeaturesAndClassification(
      "/LayoutTests/images/resources/blue-wheel-srgb-color-profile.png",
      &features));
  EXPECT_EQ(classifier()->ClassifyImageUsingDecisionTreeForTesting(features),
            HighContrastClassification::kNotClassified);
  AssertFeaturesEqual(features, {1.0f, 0.126953f, 0.0f, 0.24f});

  // Test Case 5:
  // Color
  // Color Buckets Ratio: High
  // Decision Tree: Don't Apply
  // Neural Network: N/A
  EXPECT_FALSE(GetFeaturesAndClassification(
      "/LayoutTests/images/resources/ycbcr-progressive-001.jpg", &features));
  EXPECT_EQ(classifier()->ClassifyImageUsingDecisionTreeForTesting(features),
            HighContrastClassification::kDoNotApplyHighContrastFilter);
  AssertFeaturesEqual(features, {1.0f, 0.228516f, 0.0f, 0.06f});
}

}  // namespace blink
