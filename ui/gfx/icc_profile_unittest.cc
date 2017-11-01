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
  scoped_refptr<ICCProfile> icc_profile = ICCProfileForTestingColorSpin();
  ColorSpace color_space_from_icc_profile = icc_profile->GetColorSpace();

  scoped_refptr<ICCProfile> icc_profile_from_color_space =
      color_space_from_icc_profile.GetICCProfile();
  EXPECT_TRUE(icc_profile == icc_profile_from_color_space);
}

TEST(ICCProfile, SRGB) {
  scoped_refptr<ICCProfile> icc_profile = ICCProfileForTestingSRGB();
  ColorSpace color_space = ColorSpace::CreateSRGB();
  sk_sp<SkColorSpace> sk_color_space = SkColorSpace::MakeSRGB();

  // The ICC profile parser should note that this is SRGB.
  EXPECT_EQ(icc_profile->GetColorSpace(), ColorSpace::CreateSRGB());
  EXPECT_EQ(icc_profile->GetColorSpace().ToSkColorSpace().get(),
            sk_color_space.get());
  // The parametric generating code should recognize that this is SRGB.
  EXPECT_EQ(icc_profile->GetColorSpace().GetParametricApproximation(),
            ColorSpace::CreateSRGB());
  EXPECT_EQ(icc_profile->GetColorSpace()
                .GetParametricApproximation()
                .ToSkColorSpace()
                .get(),
            sk_color_space.get());
  // The generated color space should recognize that this is SRGB.
  EXPECT_EQ(color_space.ToSkColorSpace().get(), sk_color_space.get());
}

TEST(ICCProfile, Equality) {
  scoped_refptr<ICCProfile> spin_profile = ICCProfileForTestingColorSpin();
  scoped_refptr<ICCProfile> adobe_profile = ICCProfileForTestingAdobeRGB();
  EXPECT_TRUE(spin_profile == spin_profile);
  EXPECT_FALSE(spin_profile != spin_profile);
  EXPECT_FALSE(spin_profile == adobe_profile);
  EXPECT_TRUE(spin_profile != adobe_profile);

  gfx::ColorSpace spin_space = spin_profile->GetColorSpace();
  gfx::ColorSpace adobe_space = adobe_profile->GetColorSpace();
  EXPECT_TRUE(spin_space == spin_space);
  EXPECT_FALSE(spin_space != spin_space);
  EXPECT_FALSE(spin_space == adobe_space);
  EXPECT_TRUE(spin_space != adobe_space);

  scoped_refptr<ICCProfile> temp = spin_space.GetICCProfile();
  EXPECT_TRUE(spin_profile == temp);
  EXPECT_FALSE(spin_profile != temp);

  temp = adobe_space.GetICCProfile();
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
  scoped_refptr<ICCProfile> multi_tr_fn = ICCProfileForTestingNoAnalyticTrFn();
  EXPECT_NE(multi_tr_fn->GetColorSpace(),
            multi_tr_fn->GetColorSpace().GetParametricApproximation());

  scoped_refptr<ICCProfile> multi_tr_fn_color_space =
      multi_tr_fn->GetColorSpace().GetICCProfile();
  EXPECT_EQ(multi_tr_fn_color_space, multi_tr_fn);

  scoped_refptr<ICCProfile> multi_tr_fn_parametric_color_space =
      multi_tr_fn->GetColorSpace().GetParametricApproximation().GetICCProfile();
  EXPECT_NE(multi_tr_fn_parametric_color_space, multi_tr_fn);

  // This ICC profile has a transfer function with T(1) that is greater than 1
  // in the approximation, but is still close enough to be considered accurate.
  scoped_refptr<ICCProfile> overshoot = ICCProfileForTestingOvershoot();
  EXPECT_EQ(overshoot->GetColorSpace(),
            overshoot->GetColorSpace().GetParametricApproximation());

  scoped_refptr<ICCProfile> overshoot_color_space =
      overshoot->GetColorSpace().GetICCProfile();
  EXPECT_EQ(overshoot_color_space, overshoot);

  scoped_refptr<ICCProfile> overshoot_parametric_color_space =
      overshoot->GetColorSpace().GetParametricApproximation().GetICCProfile();
  EXPECT_EQ(overshoot_parametric_color_space, overshoot);

  // This ICC profile is precisely represented by the parametric color space.
  scoped_refptr<ICCProfile> accurate = ICCProfileForTestingAdobeRGB();
  EXPECT_EQ(accurate->GetColorSpace(),
            accurate->GetColorSpace().GetParametricApproximation());

  scoped_refptr<ICCProfile> accurate_color_space =
      accurate->GetColorSpace().GetICCProfile();
  EXPECT_EQ(accurate_color_space, accurate);

  scoped_refptr<ICCProfile> accurate_parametric_color_space =
      accurate->GetColorSpace().GetParametricApproximation().GetICCProfile();
  EXPECT_EQ(accurate_parametric_color_space, accurate);

  // This ICC profile has only an A2B representation. We cannot create an
  // SkColorSpaceXform to A2B only ICC profiles, so this should be marked
  // as invalid.
  scoped_refptr<ICCProfile> a2b = ICCProfileForTestingA2BOnly();
  EXPECT_FALSE(a2b->GetColorSpace().IsValid());

  // Even though it is invalid, it should not be equal to the empty constructor.
  EXPECT_NE(a2b, nullptr);
}

TEST(ICCProfile, GarbageData) {
  std::vector<char> bad_data(10 * 1024);
  const char* bad_data_string = "deadbeef";
  for (size_t i = 0; i < bad_data.size(); ++i)
    bad_data[i] = bad_data_string[i % 8];
  scoped_refptr<ICCProfile> garbage_profile =
      ICCProfile::FromData(bad_data.data(), bad_data.size());
  EXPECT_FALSE(garbage_profile->IsValid());
  EXPECT_FALSE(garbage_profile->GetColorSpace().IsValid());
  EXPECT_FALSE(
      garbage_profile->GetColorSpace().GetParametricApproximation().IsValid());

  scoped_refptr<ICCProfile> default_ctor_profile =
      ICCProfile::FromData(nullptr, 0);
  EXPECT_FALSE(default_ctor_profile->IsValid());
  EXPECT_FALSE(default_ctor_profile->GetColorSpace().IsValid());
  EXPECT_FALSE(default_ctor_profile->GetColorSpace()
                   .GetParametricApproximation()
                   .IsValid());
}

TEST(ICCProfile, GenericRGB) {
  ColorSpace icc_profile = ICCProfileForTestingGenericRGB()->GetColorSpace();
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

// This tests the scoped_refptr<ICCProfile> MRU cache. This cache is sloppy and
// should be rewritten, now that some of the original design constraints have
// been lifted. This test exists only to ensure that we are aware of behavior
// changes, not to enforce that behavior does not change.
// https://crbug.com/766736
TEST(ICCProfile, ExhaustCache) {
  // Get an scoped_refptr<ICCProfile> that can't be parametrically approximated.
  scoped_refptr<ICCProfile> original = ICCProfileForTestingGenericRGB();
  ColorSpace original_color_space_0 = original->GetColorSpace();
  EXPECT_FALSE(ICCProfile::FromDisplayCache(original_color_space_0));
  original->AddToDisplayCache();
  EXPECT_TRUE(ICCProfile::FromDisplayCache(original_color_space_0));

  // Create 128 distinct ICC profiles, but don't add them to the cache.
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
    scoped_refptr<ICCProfile> icc_profile = color_space.GetICCProfile();
  }

  // The entry should still be in the cache.
  EXPECT_TRUE(ICCProfile::FromDisplayCache(original_color_space_0));

  // Create 128 distinct ICC profiles.
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
    scoped_refptr<ICCProfile> icc_profile = color_space.GetICCProfile();
    icc_profile->AddToDisplayCache();
  }

  // That should kick the profile out of the cache.
  EXPECT_FALSE(ICCProfile::FromDisplayCache(original_color_space_0));

  // It should be back in the cache when re-touched.
  original->AddToDisplayCache();
  EXPECT_TRUE(ICCProfile::FromDisplayCache(original_color_space_0));
}

}  // namespace gfx
