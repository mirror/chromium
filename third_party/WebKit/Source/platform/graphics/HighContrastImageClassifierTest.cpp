// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/HighContrastImageClassifier.h"

#include "base/files/file_enumerator.h"  // TODO: Remove
#include "base/files/file_path.h"        // TODO: Remove
#include "base/files/file_util.h"        // TODO: Remove
#include "base/strings/stringprintf.h"   // TODO: Remove
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

// TEST_F(HighContrastImageClassifierTest, ShouldApplyHighContrastFilterToImage)
// {
//   RefPtr<BitmapImage> image = LoadImage(
//       "/LayoutTests/images/resources/blue-wheel-srgb-color-profile.png");
//   EXPECT_FALSE(classifier_.ShouldApplyHighContrastFilterToImage(*image.get()));

//   image = LoadImage("/LayoutTests/images/resources/grid.png");
//   EXPECT_TRUE(classifier_.ShouldApplyHighContrastFilterToImage(*image.get()));
// }

TEST_F(HighContrastImageClassifierTest, GenerateSamples) {
  base::FilePath FeaturesFile(
      "/usr/local/google/home/rhalavati/workspace/highcontrast/features.csv");
  base::FilePath SamplesFolder(
      "/usr/local/google/home/rhalavati/workspace/highcontrast/samples");

  std::string features_string;
  base::FileEnumerator enumerator(SamplesFolder, false,
                                  base::FileEnumerator::FILES);
  while (true) {
    const std::string file_path = enumerator.Next().MaybeAsASCII();
    if (file_path.empty())
      break;

    std::vector<float> features;
    RefPtr<SharedBuffer> image_data = testing::ReadFromFile(file_path.c_str());
    EXPECT_TRUE(image_data.get());

    RefPtr<BitmapImage> image = BitmapImage::Create();
    image->SetData(image_data, true);
    if (classifier_.ComputeImageFeaturesForTesting(*image.get(), &features)) {
      for (float feature : features)
        features_string += base::StringPrintf("%f,", feature);
      features_string += base::StringPrintf(
          "%i,%s\n", file_path.find("pos_") != std::string::npos,
          file_path.c_str());
      LOG(INFO) << file_path;
    } else {
      LOG(ERROR) << file_path;
    }
  }

  EXPECT_GT(base::WriteFile(FeaturesFile, features_string.c_str(),
                            features_string.length()),
            0);
}

}  // namespace blink
