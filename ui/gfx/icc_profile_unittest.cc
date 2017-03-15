// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/color_space.h"
#include "ui/gfx/icc_profile.h"
#include "ui/gfx/test/icc_profiles.h"

namespace gfx {

TEST(ICCProfile, Conversions) {
  ICCProfile icc_profile = ICCProfileForTestingColorSpin();
  ColorSpace color_space_from_icc_profile = icc_profile.GetColorSpace();

  ICCProfile icc_profile_from_color_space;
  bool result =
      color_space_from_icc_profile.GetICCProfile(&icc_profile_from_color_space);
  EXPECT_TRUE(result);
  EXPECT_TRUE(icc_profile == icc_profile_from_color_space);
}

TEST(ICCProfile, SRGB) {
  ColorSpace color_space = ColorSpace::CreateSRGB();
  sk_sp<SkColorSpace> sk_color_space = SkColorSpace::MakeSRGB();

  // These should be the same pointer, not just equal.
  EXPECT_EQ(color_space.ToSkColorSpace().get(), sk_color_space.get());
}

TEST(ICCProfile, Equality) {
  ICCProfile spin_profile = ICCProfileForTestingColorSpin();
  ICCProfile adobe_profile = ICCProfileForTestingAdobeRGB();
  EXPECT_TRUE(spin_profile == spin_profile);
  EXPECT_FALSE(spin_profile != spin_profile);
  EXPECT_FALSE(spin_profile == adobe_profile);
  EXPECT_TRUE(spin_profile != adobe_profile);

  gfx::ColorSpace spin_space = spin_profile.GetColorSpace();
  gfx::ColorSpace adobe_space = adobe_profile.GetColorSpace();
  EXPECT_TRUE(spin_space == spin_space);
  EXPECT_FALSE(spin_space != spin_space);
  EXPECT_FALSE(spin_space == adobe_space);
  EXPECT_TRUE(spin_space != adobe_space);

  ICCProfile temp;
  bool get_icc_result = false;

  get_icc_result = spin_space.GetICCProfile(&temp);
  EXPECT_TRUE(get_icc_result);
  EXPECT_TRUE(spin_profile == temp);
  EXPECT_FALSE(spin_profile != temp);

  get_icc_result = adobe_space.GetICCProfile(&temp);
  EXPECT_TRUE(get_icc_result);
  EXPECT_FALSE(spin_profile == temp);
  EXPECT_TRUE(spin_profile != temp);

  EXPECT_TRUE(!!spin_space.ToSkColorSpace());
  EXPECT_TRUE(!!adobe_space.ToSkColorSpace());
  EXPECT_FALSE(SkColorSpace::Equals(
      spin_space.ToSkColorSpace().get(),
      adobe_space.ToSkColorSpace().get()));
}

TEST(ICCProfile, ParametricVersusExact) {
  // This ICC profile has three transfer functions that differ enough that the
  // parametric color space is considered inaccurate.
  ICCProfile inaccurate = ICCProfileForTestingNoAnalyticTrFn();
  EXPECT_NE(inaccurate.GetColorSpace(), inaccurate.GetParametricColorSpace());

  ICCProfile inaccurate_color_space;
  EXPECT_TRUE(
      inaccurate.GetColorSpace().GetICCProfile(&inaccurate_color_space));
  EXPECT_EQ(inaccurate_color_space, inaccurate);

  ICCProfile inaccurate_parametric_color_space;
  EXPECT_TRUE(inaccurate.GetParametricColorSpace().GetICCProfile(
      &inaccurate_parametric_color_space));
  EXPECT_NE(inaccurate_parametric_color_space, inaccurate);

  // This ICC profile is precisely represented by the parametric color space.
  ICCProfile accurate = ICCProfileForTestingAdobeRGB();
  EXPECT_EQ(accurate.GetColorSpace(), accurate.GetParametricColorSpace());

  ICCProfile accurate_color_space;
  EXPECT_TRUE(accurate.GetColorSpace().GetICCProfile(&accurate_color_space));
  EXPECT_EQ(accurate_color_space, accurate);

  ICCProfile accurate_parametric_color_space;
  EXPECT_TRUE(accurate.GetParametricColorSpace().GetICCProfile(
      &accurate_parametric_color_space));
  EXPECT_EQ(accurate_parametric_color_space, accurate);

  // This ICC profile has only an A2B representation. For now, this means that
  // the parametric representation will be sRGB. We will eventually want to get
  // some sense of the gamut.
  ICCProfile a2b = ICCProfileForTestingA2BOnly();
  EXPECT_TRUE(a2b.GetColorSpace().IsValid());
  EXPECT_NE(a2b.GetColorSpace(), a2b.GetParametricColorSpace());
  EXPECT_EQ(ColorSpace::CreateSRGB(), a2b.GetParametricColorSpace());
}

TEST(ICCProfile, GarbageData) {
  std::vector<char> bad_data(10 * 1024);
  const char* bad_data_string = "deadbeef";
  for (size_t i = 0; i < bad_data.size(); ++i)
    bad_data[i] = bad_data_string[i % 8];
  ICCProfile garbage_profile =
      ICCProfile::FromData(bad_data.data(), bad_data.size());
  EXPECT_FALSE(garbage_profile.IsValid());
  EXPECT_FALSE(garbage_profile.GetColorSpace().IsValid());
  EXPECT_FALSE(garbage_profile.GetParametricColorSpace().IsValid());

  ICCProfile default_ctor_profile;
  EXPECT_FALSE(default_ctor_profile.IsValid());
  EXPECT_FALSE(default_ctor_profile.GetColorSpace().IsValid());
  EXPECT_FALSE(default_ctor_profile.GetParametricColorSpace().IsValid());
}

}  // namespace gfx
