// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiles/profile_avatar_icon_util.h"

#include "chrome/grit/theme_resources.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_rep.h"
#include "ui/gfx/image/image_unittest_util.h"
#include "url/gurl.h"

namespace {

// Helper function to check that the image is sized properly
// and supports multiple pixel densities.
void VerifyScaling(gfx::Image& image, gfx::Size& size) {
  gfx::Size canvas_size(100, 100);
  gfx::Canvas canvas(canvas_size, 1.0f, false);
  gfx::Canvas canvas2(canvas_size, 2.0f, false);

  ASSERT_FALSE(gfx::test::IsEmpty(image));
  EXPECT_EQ(image.Size(), size);

  gfx::ImageSkia image_skia = *image.ToImageSkia();
  canvas.DrawImageInt(image_skia, 15, 10);
  EXPECT_TRUE(image.ToImageSkia()->HasRepresentation(1.0f));

  canvas2.DrawImageInt(image_skia, 15, 10);
  EXPECT_TRUE(image.ToImageSkia()->HasRepresentation(2.0f));
}

TEST(ProfileInfoUtilTest, SizedMenuIcon) {
  // Test that an avatar icon isn't changed.
  const gfx::Image& profile_image(
      ResourceBundle::GetSharedInstance().GetImageNamed(IDR_PROFILE_AVATAR_0));
  gfx::Image result =
      profiles::GetSizedAvatarIcon(profile_image, false, 50, 50);

  EXPECT_FALSE(gfx::test::IsEmpty(result));
  EXPECT_TRUE(gfx::test::AreImagesEqual(profile_image, result));

  // Test that a rectangular picture (e.g., GAIA image) is changed.
  gfx::Image rect_picture(gfx::test::CreateImage());

  gfx::Size size(30, 20);
  gfx::Image result2 = profiles::GetSizedAvatarIcon(
      rect_picture, true, size.width(), size.height());

  VerifyScaling(result2, size);
}

TEST(ProfileInfoUtilTest, MenuIcon) {
  // Test that an avatar icon isn't changed.
  const gfx::Image& profile_image(
      ResourceBundle::GetSharedInstance().GetImageNamed(IDR_PROFILE_AVATAR_0));
  gfx::Image result = profiles::GetAvatarIconForMenu(profile_image, false);
  EXPECT_FALSE(gfx::test::IsEmpty(result));
  EXPECT_TRUE(gfx::test::AreImagesEqual(profile_image, result));

  // Test that a rectangular picture is changed.
  gfx::Image rect_picture(gfx::test::CreateImage());
  gfx::Size size(profiles::kAvatarIconWidth, profiles::kAvatarIconHeight);
  gfx::Image result2 = profiles::GetAvatarIconForMenu(rect_picture, true);

  VerifyScaling(result2, size);
}

TEST(ProfileInfoUtilTest, WebUIIcon) {
  // Test that an avatar icon isn't changed.
  const gfx::Image& profile_image(
      ResourceBundle::GetSharedInstance().GetImageNamed(IDR_PROFILE_AVATAR_0));
  gfx::Image result = profiles::GetAvatarIconForWebUI(profile_image, false);
  EXPECT_FALSE(gfx::test::IsEmpty(result));
  EXPECT_TRUE(gfx::test::AreImagesEqual(profile_image, result));

  // Test that a rectangular picture is changed.
  gfx::Image rect_picture(gfx::test::CreateImage());
  gfx::Size size(profiles::kAvatarIconWidth, profiles::kAvatarIconHeight);
  gfx::Image result2 = profiles::GetAvatarIconForWebUI(rect_picture, true);

  VerifyScaling(result2, size);
}

TEST(ProfileInfoUtilTest, TitleBarIcon) {
  int width = 100;
  int height = 40;

  // Test that an avatar icon isn't changed.
  const gfx::Image& profile_image(
      ResourceBundle::GetSharedInstance().GetImageNamed(IDR_PROFILE_AVATAR_0));
  gfx::Image result = profiles::GetAvatarIconForTitleBar(
      profile_image, false, width, height);
  EXPECT_FALSE(gfx::test::IsEmpty(result));
  EXPECT_TRUE(gfx::test::AreImagesEqual(profile_image, result));

  // Test that a rectangular picture is changed.
  gfx::Image rect_picture(gfx::test::CreateImage());

  gfx::Size size(width, height);
  gfx::Image result2 = profiles::GetAvatarIconForTitleBar(
      rect_picture, true, width, height);

  VerifyScaling(result2, size);
}

void TestGetImageURLWithOptions(const char* initial_url,
                                int size,
                                bool no_silhouette,
                                const char* expected_url) {
  GURL transformed_url;
  EXPECT_TRUE(profiles::GetImageURLWithOptions(
      GURL(initial_url), size, no_silhouette, &transformed_url));
  EXPECT_EQ(transformed_url, GURL(expected_url));
}

TEST(ProfileInfoUtilTest, GetImageURLWithOptionsNoInitialSize) {
  EXPECT_NO_FATAL_FAILURE(TestGetImageURLWithOptions(
      "http://example.com/-A/AAAAAAAAAAI/AAAAAAAAACQ/B/photo.jpg", 128,
      false /* no_silhouette */,
      "http://example.com/-A/AAAAAAAAAAI/AAAAAAAAACQ/B/s128-c/photo.jpg"));
}

TEST(ProfileInfoUtilTest, GetImageURLWithOptionsSizeAlreadySpecified) {
  // If there's already a size specified in the URL, it should be changed to the
  // specified size in the resulting URL.
  EXPECT_NO_FATAL_FAILURE(TestGetImageURLWithOptions(
      "http://example.com/-A/AAAAAAAAAAI/AAAAAAAAACQ/B/s64-c/photo.jpg", 128,
      false /* no_silhouette */,
      "http://example.com/-A/AAAAAAAAAAI/AAAAAAAAACQ/B/s128-c/photo.jpg"));
}

TEST(ProfileInfoUtilTest, GetImageURLWithOptionsOtherSizeSpecified) {
  // If there's already a size specified in the URL, it should be changed to the
  // specified size in the resulting URL.
  EXPECT_NO_FATAL_FAILURE(TestGetImageURLWithOptions(
      "http://example.com/-A/AAAAAAAAAAI/AAAAAAAAACQ/B/s128-c/photo.jpg", 64,
      false /* no_silhouette */,
      "http://example.com/-A/AAAAAAAAAAI/AAAAAAAAACQ/B/s64-c/photo.jpg"));
}

TEST(ProfileInfoUtilTest, GetImageURLWithOptionsSameSize) {
  // If there's already a size specified in the URL, and it's already the
  // requested size, true should be returned and the URL should remain
  // unchanged.
  EXPECT_NO_FATAL_FAILURE(TestGetImageURLWithOptions(
      "http://example.com/-A/AAAAAAAAAAI/AAAAAAAAACQ/B/s64-c/photo.jpg", 64,
      false /* no_silhouette */,
      "http://example.com/-A/AAAAAAAAAAI/AAAAAAAAACQ/B/s64-c/photo.jpg"));
}

TEST(ProfileInfoUtilTest, GetImageURLWithOptionsNoFileNameInPath) {
  // If there is no file path component in the URL path, we should fail the
  // modification, but return true since the URL is still potentially valid and
  // not modify the input URL.
  EXPECT_NO_FATAL_FAILURE(TestGetImageURLWithOptions(
      "http://example.com/-A/AAAAAAAAAAI/AAAAAAAAACQ/B/", 128,
      false /* no_silhouette */,
      "http://example.com/-A/AAAAAAAAAAI/AAAAAAAAACQ/B/"));
}

TEST(ProfileInfoUtilTest, GetImageURLWithOptionsNoSilhouette) {
  // If there's already a size specified in the URL, it should be changed to the
  // specified size in the resulting URL.
  EXPECT_NO_FATAL_FAILURE(TestGetImageURLWithOptions(
      "http://example.com/-A/AAAAAAAAAAI/AAAAAAAAACQ/B/photo.jpg", 64,
      true /* no_silhouette */,
      "http://example.com/-A/AAAAAAAAAAI/AAAAAAAAACQ/B/s64-c-ns/photo.jpg"));
}

TEST(ProfileInfoUtilTest, GetImageURLWithOptionsSizeReplaceNoSilhouette) {
  // If there's already a no_silhouette option specified in the URL, it should
  // be removed if necessary.
  EXPECT_NO_FATAL_FAILURE(TestGetImageURLWithOptions(
      "http://example.com/-A/AAAAAAAAAAI/AAAAAAAAACQ/B/s64-c-ns/photo.jpg", 128,
      false /* no_silhouette */,
      "http://example.com/-A/AAAAAAAAAAI/AAAAAAAAACQ/B/s128-c/photo.jpg"));
}

TEST(ProfileInfoUtilTest, GetImageURLWithOptionsUnknownShouldBePreserved) {
  // If there are any unknown options encoded in URL, GetImageURLWithOptions
  // should preserve them.
  EXPECT_NO_FATAL_FAILURE(TestGetImageURLWithOptions(
      "http://example.com/-A/AAAAAAAAAAI/AAAAAAAAACQ/B/s64-mo-k/photo.jpg", 128,
      false /* no_silhouette */,
      "http://example.com/-A/AAAAAAAAAAI/AAAAAAAAACQ/B/mo-k-s128-c/photo.jpg"));
}

TEST(ProfileInfoUtilTest, GetImageURLWithThumbnailShouldNotChangeURLIfInvalid) {
  GURL initial_url;
  GURL new_url("http://example.com");
  EXPECT_FALSE(profiles::GetImageURLWithOptions(
      initial_url, 128, false /* no_silhouette */, &new_url));

  // The new URL should be unchanged since the transformation failed.
  EXPECT_EQ(new_url, GURL("http://example.com"));
}

}  // namespace
