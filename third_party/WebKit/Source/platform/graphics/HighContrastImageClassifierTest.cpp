// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/HighContrastImageClassifier.h"

#include "platform/SharedBuffer.h"
#include "platform/graphics/BitmapImage.h"
#include "platform/testing/TestingPlatformSupport.h"
#include "platform/testing/UnitTestHelpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class HighContrastImageClassifierTest : public ::testing::Test {
 public:
  RefPtr<BitmapImage> LoadImage(const char* file_name) {
    String file_path = testing::BlinkRootDir();
    file_path.append(file_name);
    RefPtr<SharedBuffer> image_data = testing::ReadFromFile(file_path);
    EXPECT_TRUE(image_data.get());

    RefPtr<BitmapImage> image = BitmapImage::Create();
    image->SetData(image_data, true);
    return image;
  }

 protected:
  ScopedTestingPlatformSupport<TestingPlatformSupportWithMockScheduler>
      platform_;
  HighContrastImageClassifier classifier_;
};

TEST_F(HighContrastImageClassifierTest, FeaturesAndClassification) {
  struct TestCase {
    std::string file_name;
    bool expected_result;
    float features[2];
  };

  TestCase test_cases[] = {
      {"/LayoutTests/images/resources/blue-wheel-srgb-color-profile.png",
       false,
       {1, 0.0366914}},
      {"/LayoutTests/images/resources/grid-large.png", true, {0, 0.1875}},
  };

  for (const auto& test_case : test_cases) {
    SCOPED_TRACE(test_case.file_name);
    RefPtr<BitmapImage> image = LoadImage(test_case.file_name.c_str());
    std::vector<float> features;
    EXPECT_TRUE(
        classifier_.ComputeImageFeaturesForTesting(*image.get(), &features));

    EXPECT_EQ(classifier_.ShouldApplyHighContrastFilterToImage(*image.get()),
              test_case.expected_result);
  }
}

}  // namespace blink
