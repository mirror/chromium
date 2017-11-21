// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/icc_profile.h"
#include "base/logging.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/color_space.h"
#include "ui/gfx/skia_color_space_util.h"
#include "ui/gfx/test/icc_profiles.h"

namespace gfx {

TEST(ICCProfile, Conversions) {
  ICCProfile icc_profile = ICCProfileForTestingColorSpin();
  ColorSpace color_space_from_icc_profile = icc_profile.GetColorSpace();

  ICCProfile icc_profile_from_color_space =
      ICCProfile::FromColorSpace(color_space_from_icc_profile);
  EXPECT_TRUE(icc_profile_from_color_space.IsValid());
  EXPECT_TRUE(icc_profile == icc_profile_from_color_space);
}

TEST(ICCProfile, SRGB) {
  ICCProfile icc_profile = ICCProfileForTestingSRGB();
  ColorSpace color_space = ColorSpace::CreateSRGB();
  sk_sp<SkColorSpace> sk_color_space = SkColorSpace::MakeSRGB();

  // The ICC profile parser should note that this is SRGB.
  EXPECT_EQ(icc_profile.GetColorSpace(), ColorSpace::CreateSRGB());
  EXPECT_EQ(icc_profile.GetColorSpace().ToSkColorSpace().get(),
            sk_color_space.get());
  // The parametric generating code should recognize that this is SRGB.
  EXPECT_EQ(icc_profile.GetParametricColorSpace(), ColorSpace::CreateSRGB());
  EXPECT_EQ(icc_profile.GetParametricColorSpace().ToSkColorSpace().get(),
            sk_color_space.get());
  // The generated color space should recognize that this is SRGB.
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

  ICCProfile temp = ICCProfile::FromColorSpace(spin_space);
  EXPECT_TRUE(temp.IsValid());
  EXPECT_TRUE(spin_profile == temp);
  EXPECT_FALSE(spin_profile != temp);

  temp = ICCProfile::FromColorSpace(adobe_space);
  EXPECT_TRUE(temp.IsValid());
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
  ICCProfile multi_tr_fn = ICCProfileForTestingNoAnalyticTrFn();
  EXPECT_NE(multi_tr_fn.GetColorSpace(), multi_tr_fn.GetParametricColorSpace());

  ICCProfile multi_tr_fn_color_space =
      ICCProfile::FromColorSpace(multi_tr_fn.GetColorSpace());
  EXPECT_TRUE(multi_tr_fn_color_space.IsValid());
  EXPECT_EQ(multi_tr_fn_color_space, multi_tr_fn);

  ICCProfile multi_tr_fn_parametric_color_space =
      ICCProfile::FromColorSpace(multi_tr_fn.GetParametricColorSpace());
  EXPECT_TRUE(multi_tr_fn_parametric_color_space.IsValid());
  EXPECT_NE(multi_tr_fn_parametric_color_space, multi_tr_fn);

  // This ICC profile has a transfer function with T(1) that is greater than 1
  // in the approximation, but is still close enough to be considered accurate.
  ICCProfile overshoot = ICCProfileForTestingOvershoot();
  EXPECT_EQ(overshoot.GetColorSpace(), overshoot.GetParametricColorSpace());

  ICCProfile overshoot_color_space =
      ICCProfile::FromColorSpace(overshoot.GetColorSpace());
  EXPECT_TRUE(overshoot_color_space.IsValid());
  EXPECT_EQ(overshoot_color_space, overshoot);

  ICCProfile overshoot_parametric_color_space =
      ICCProfile::FromColorSpace(overshoot.GetParametricColorSpace());
  EXPECT_TRUE(overshoot_parametric_color_space.IsValid());
  EXPECT_EQ(overshoot_parametric_color_space, overshoot);

  // This ICC profile is precisely represented by the parametric color space.
  ICCProfile accurate = ICCProfileForTestingAdobeRGB();
  EXPECT_EQ(accurate.GetColorSpace(), accurate.GetParametricColorSpace());

  ICCProfile accurate_color_space =
      ICCProfile::FromColorSpace(accurate.GetColorSpace());
  EXPECT_TRUE(accurate_color_space.IsValid());
  EXPECT_EQ(accurate_color_space, accurate);

  ICCProfile accurate_parametric_color_space =
      ICCProfile::FromColorSpace(accurate.GetParametricColorSpace());
  EXPECT_TRUE(accurate_parametric_color_space.IsValid());
  EXPECT_EQ(accurate_parametric_color_space, accurate);

  // This ICC profile has only an A2B representation. We cannot create an
  // SkColorSpaceXform to A2B only ICC profiles, so this should be marked
  // as invalid.
  ICCProfile a2b = ICCProfileForTestingA2BOnly();
  EXPECT_FALSE(a2b.GetColorSpace().IsValid());

  // Even though it is invalid, it should not be equal to the empty constructor.
  EXPECT_NE(a2b, gfx::ICCProfile());
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

TEST(ICCProfile, GenericRGB) {
  ColorSpace icc_profile = ICCProfileForTestingGenericRGB().GetColorSpace();
  ColorSpace color_space(ColorSpace::PrimaryID::APPLE_GENERIC_RGB,
                         ColorSpace::TransferID::GAMMA18);

  SkMatrix44 icc_profile_matrix;
  SkMatrix44 color_space_matrix;

  icc_profile.GetPrimaryMatrix(&icc_profile_matrix);
  color_space.GetPrimaryMatrix(&color_space_matrix);

  SkMatrix44 eye;
  icc_profile_matrix.invert(&eye);
  eye.postConcat(color_space_matrix);
  EXPECT_TRUE(SkMatrixIsApproximatelyIdentity(eye));
}

// This tests the ICCProfile MRU cache. This cache is sloppy and should be
// rewritten, now that some of the original design constraints have been lifted.
// This test exists only to ensure that we are aware of behavior changes, not to
// enforce that behavior does not change.
// https://crbug.com/766736
TEST(ICCProfile, ExhaustCache) {
  // Get an ICCProfile that can't be parametrically approximated.
  ICCProfile original = ICCProfileForTestingNoAnalyticTrFn();
  ColorSpace original_color_space_0 = original.GetColorSpace();

  // Recover the ICCProfile from its GetColorSpace. Recovery should succeed, and
  // the ICCProfiles should be equal.
  ICCProfile recovered_0 = ICCProfile::FromColorSpace(original_color_space_0);
  EXPECT_TRUE(recovered_0.IsValid());
  EXPECT_EQ(original, recovered_0);

  // The GetColorSpace of the recovered version should match the original.
  ColorSpace recovered_0_color_space = recovered_0.GetColorSpace();
  EXPECT_EQ(recovered_0_color_space, original_color_space_0);

  // Create an identical ICCProfile to the original. It should equal the
  // original, and its GetColorSpace should equal the original.
  ICCProfile identical_0 = ICCProfileForTestingNoAnalyticTrFn();
  EXPECT_EQ(original, identical_0);
  ColorSpace identical_color_space_0 = identical_0.GetColorSpace();
  EXPECT_EQ(identical_color_space_0, original_color_space_0);

  // Create 128 distinct ICC profiles. This will destroy the cached
  // ICCProfile<->ColorSpace mapping.
  for (size_t i = 0; i < 128; ++i) {
    SkMatrix44 toXYZD50;
    ColorSpace::CreateSRGB().GetPrimaryMatrix(&toXYZD50);
    SkColorSpaceTransferFn fn;
    fn.fA = 1;
    fn.fB = 0;
    fn.fC = 1;
    fn.fD = 0;
    fn.fE = 0;
    fn.fF = 0;
    fn.fG = 1.5f + i / 128.f;
    ColorSpace color_space = ColorSpace::CreateCustom(toXYZD50, fn);
    ICCProfile::FromColorSpace(color_space);
  }

  // Recover the original ICCProfile from its GetColorSpace. Recovery should
  // fail, because it has been pushed out of the cache.
  ICCProfile recovered_1 = ICCProfile::FromColorSpace(original_color_space_0);
  EXPECT_FALSE(recovered_1.IsValid());
  EXPECT_NE(original, recovered_1);

  // Create an identical ICCProfile to the original. It will not be equal to
  // the original, because they will have different ids.
  ICCProfile identical_1 = ICCProfileForTestingNoAnalyticTrFn();
  EXPECT_NE(original, identical_1);

  // The two ICCProfiles (with the same data) will produce color spaces that
  // have different profile ids (and so are unequal).
  ColorSpace identical_1_color_space = identical_1.GetColorSpace();
  EXPECT_NE(identical_1_color_space, original_color_space_0);

  // The original ICCProfile is now orphaned because there exists a new entry
  // with the same data.
  ColorSpace original_color_space_2 = original.GetColorSpace();
  ICCProfile recovered_2 = ICCProfile::FromColorSpace(original_color_space_2);
  EXPECT_FALSE(recovered_2.IsValid());
  EXPECT_NE(original, recovered_2);

  // Blow away the cache one more time.
  for (size_t i = 0; i < 128; ++i) {
    SkMatrix44 toXYZD50;
    ColorSpace::CreateSRGB().GetPrimaryMatrix(&toXYZD50);
    SkColorSpaceTransferFn fn;
    fn.fA = 1;
    fn.fB = 0;
    fn.fC = 1;
    fn.fD = 0;
    fn.fE = 0;
    fn.fF = 0;
    fn.fG = 1.5f + i / 128.f;
    ColorSpace color_space = ColorSpace::CreateCustom(toXYZD50, fn);
    ICCProfile::FromColorSpace(color_space);
  }

  // The original ICCProfile remains out of the cache.
  ColorSpace original_color_space_3 = original.GetColorSpace();
  ICCProfile recovered_3 = ICCProfile::FromColorSpace(original_color_space_3);
  EXPECT_FALSE(recovered_3.IsValid());
  EXPECT_NE(original, recovered_3);
}

}  // namespace gfx
