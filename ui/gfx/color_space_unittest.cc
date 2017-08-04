// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cmath>

#include "base/logging.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/color_space.h"
#include "ui/gfx/skia_color_space_util.h"

namespace gfx {
namespace {

const float kEpsilon = 1.0e-3f;

// Returns the L-infty difference of u and v.
float Diff(const SkVector4& u, const SkVector4& v) {
  float result = 0;
  for (size_t i = 0; i < 4; ++i)
    result = std::max(result, std::abs(u.fData[i] - v.fData[i]));
  return result;
}

TEST(ColorSpace, RGBToYUV) {
  const size_t kNumTestRGBs = 3;
  SkVector4 test_rgbs[kNumTestRGBs] = {
      SkVector4(1.f, 0.f, 0.f, 1.f),
      SkVector4(0.f, 1.f, 0.f, 1.f),
      SkVector4(0.f, 0.f, 1.f, 1.f),
  };

  const size_t kNumColorSpaces = 4;
  gfx::ColorSpace color_spaces[kNumColorSpaces] = {
      gfx::ColorSpace::CreateREC601(), gfx::ColorSpace::CreateREC709(),
      gfx::ColorSpace::CreateJpeg(), gfx::ColorSpace::CreateXYZD50(),
  };

  SkVector4 expected_yuvs[kNumColorSpaces][kNumTestRGBs] = {
      // REC601
      {
          SkVector4(0.3195f, 0.3518f, 0.9392f, 1.0000f),
          SkVector4(0.5669f, 0.2090f, 0.1322f, 1.0000f),
          SkVector4(0.1607f, 0.9392f, 0.4286f, 1.0000f),
      },
      // REC709
      {
          SkVector4(0.2453f, 0.3994f, 0.9392f, 1.0000f),
          SkVector4(0.6770f, 0.1614f, 0.1011f, 1.0000f),
          SkVector4(0.1248f, 0.9392f, 0.4597f, 1.0000f),
      },
      // Jpeg
      {
          SkVector4(0.2990f, 0.3313f, 1.0000f, 1.0000f),
          SkVector4(0.5870f, 0.1687f, 0.0813f, 1.0000f),
          SkVector4(0.1140f, 1.0000f, 0.4187f, 1.0000f),
      },
      // XYZD50
      {
          SkVector4(1.0000f, 0.0000f, 0.0000f, 1.0000f),
          SkVector4(0.0000f, 1.0000f, 0.0000f, 1.0000f),
          SkVector4(0.0000f, 0.0000f, 1.0000f, 1.0000f),
      },
  };

  for (size_t i = 0; i < kNumColorSpaces; ++i) {
    SkMatrix44 transfer;
    color_spaces[i].GetTransferMatrix(&transfer);

    SkMatrix44 range_adjust;
    color_spaces[i].GetRangeAdjustMatrix(&range_adjust);

    SkMatrix44 range_adjust_inv;
    range_adjust.invert(&range_adjust_inv);

    for (size_t j = 0; j < kNumTestRGBs; ++j) {
      SkVector4 yuv = range_adjust_inv * transfer * test_rgbs[j];
      EXPECT_LT(Diff(yuv, expected_yuvs[i][j]), kEpsilon);
    }
  }
}

typedef std::tr1::tuple<ColorSpace::TransferID, size_t> TableTestData;

class ColorSpaceTableTest : public testing::TestWithParam<TableTestData> {};

TEST_P(ColorSpaceTableTest, ApproximateTransferFn) {
  ColorSpace::TransferID transfer_id = std::tr1::get<0>(GetParam());
  const size_t table_size = std::tr1::get<1>(GetParam());

  gfx::ColorSpace color_space(ColorSpace::PrimaryID::BT709, transfer_id);
  SkColorSpaceTransferFn tr_fn;
  SkColorSpaceTransferFn tr_fn_inv;
  bool result = color_space.GetTransferFunction(&tr_fn);
  CHECK(result);
  color_space.GetInverseTransferFunction(&tr_fn_inv);

  std::vector<float> x;
  std::vector<float> t;
  for (float v = 0; v <= 1.f; v += 1.f / table_size) {
    x.push_back(v);
    t.push_back(SkTransferFnEval(tr_fn, v));
  }

  SkColorSpaceTransferFn fn_approx;
  bool converged =
      SkApproximateTransferFn(x.data(), t.data(), x.size(), &fn_approx);
  EXPECT_TRUE(converged);

  for (size_t i = 0; i < x.size(); ++i) {
    float fn_approx_of_x = SkTransferFnEval(fn_approx, x[i]);
    EXPECT_NEAR(t[i], fn_approx_of_x, 3.f / 256.f);
    if (std::abs(t[i] - fn_approx_of_x) > 3.f / 256.f)
      break;
  }
}

ColorSpace::TransferID all_transfers[] = {
    ColorSpace::TransferID::GAMMA22,   ColorSpace::TransferID::GAMMA24,
    ColorSpace::TransferID::GAMMA28,   ColorSpace::TransferID::BT709,
    ColorSpace::TransferID::SMPTE240M, ColorSpace::TransferID::IEC61966_2_1,
    ColorSpace::TransferID::LINEAR};

size_t all_table_sizes[] = {512, 256, 128, 64, 16, 11, 8, 7, 6, 5, 4};

INSTANTIATE_TEST_CASE_P(A,
                        ColorSpaceTableTest,
                        testing::Combine(testing::ValuesIn(all_transfers),
                                         testing::ValuesIn(all_table_sizes)));

TEST(ColorSpace, ApproximateTransferFnClamped) {
  // These data represent a transfer function that is clamped at the high
  // end of its domain. It comes from the color profile attached to
  // https://crbug.com/750459
  float t[] = {
      0.000000, 0.000305, 0.000610, 0.000916, 0.001221, 0.001511, 0.001816,
      0.002121, 0.002426, 0.002731, 0.003037, 0.003601, 0.003937, 0.004303,
      0.004685, 0.005081, 0.005509, 0.005951, 0.006409, 0.006882, 0.007385,
      0.007904, 0.008438, 0.009003, 0.009583, 0.010193, 0.010819, 0.011460,
      0.012131, 0.012818, 0.013535, 0.014267, 0.015030, 0.015808, 0.016617,
      0.017456, 0.018296, 0.019181, 0.020081, 0.021012, 0.021958, 0.022934,
      0.023926, 0.024949, 0.026001, 0.027070, 0.028168, 0.029297, 0.030442,
      0.031617, 0.032822, 0.034058, 0.035309, 0.036591, 0.037903, 0.039231,
      0.040604, 0.041993, 0.043412, 0.044846, 0.046326, 0.047822, 0.049348,
      0.050904, 0.052491, 0.054108, 0.055756, 0.057420, 0.059113, 0.060853,
      0.062608, 0.064393, 0.066209, 0.068055, 0.069932, 0.071839, 0.073762,
      0.075731, 0.077729, 0.079759, 0.081804, 0.083894, 0.086015, 0.088167,
      0.090333, 0.092546, 0.094789, 0.097063, 0.099367, 0.101701, 0.104067,
      0.106477, 0.108904, 0.111360, 0.113863, 0.116381, 0.118944, 0.121538,
      0.124163, 0.126818, 0.129519, 0.132235, 0.134997, 0.137789, 0.140612,
      0.143465, 0.146365, 0.149279, 0.152239, 0.155230, 0.158267, 0.161318,
      0.164416, 0.167544, 0.170718, 0.173907, 0.177142, 0.180407, 0.183719,
      0.187045, 0.190433, 0.193835, 0.197284, 0.200763, 0.204273, 0.207813,
      0.211398, 0.215030, 0.218692, 0.222385, 0.226108, 0.229877, 0.233677,
      0.237522, 0.241382, 0.245304, 0.249256, 0.253239, 0.257252, 0.261311,
      0.265415, 0.269551, 0.273716, 0.277928, 0.282170, 0.286458, 0.290776,
      0.295140, 0.299535, 0.303975, 0.308446, 0.312947, 0.317494, 0.322087,
      0.326711, 0.331380, 0.336080, 0.340826, 0.345602, 0.350423, 0.355291,
      0.360174, 0.365118, 0.370092, 0.375113, 0.380163, 0.385260, 0.390387,
      0.395560, 0.400778, 0.406027, 0.411322, 0.416663, 0.422034, 0.427451,
      0.432898, 0.438392, 0.443931, 0.449500, 0.455116, 0.460777, 0.466468,
      0.472221, 0.477989, 0.483818, 0.489677, 0.495583, 0.501518, 0.507500,
      0.513527, 0.519600, 0.525719, 0.531868, 0.538064, 0.544289, 0.550576,
      0.556893, 0.563256, 0.569650, 0.576104, 0.582589, 0.589120, 0.595697,
      0.602304, 0.608972, 0.615671, 0.622415, 0.629206, 0.636027, 0.642908,
      0.649821, 0.656779, 0.663783, 0.670832, 0.677913, 0.685054, 0.692226,
      0.699443, 0.706706, 0.714015, 0.721370, 0.728771, 0.736202, 0.743694,
      0.751217, 0.758785, 0.766400, 0.774060, 0.781765, 0.789517, 0.797314,
      0.805158, 0.813031, 0.820966, 0.828946, 0.836957, 0.845029, 0.853132,
      0.861280, 0.869490, 0.877729, 0.886015, 0.894362, 0.902739, 0.911162,
      0.919631, 0.928161, 0.936721, 0.945327, 0.953994, 0.962692, 0.971435,
      0.980240, 0.989075, 0.997955, 1.000000,
  };
  std::vector<float> x;
  for (size_t v = 0; v < 256; ++v)
    x.push_back(v / 255.f);

  SkColorSpaceTransferFn fn_approx;
  bool converged = SkApproximateTransferFn(x.data(), t, x.size(), &fn_approx);
  EXPECT_TRUE(converged);

  // The approximation should be nearly exact.
  float expected_error = 1.f / 4096.f;
  for (size_t i = 0; i < x.size(); ++i) {
    float fn_approx_of_x = SkTransferFnEval(fn_approx, x[i]);
    EXPECT_NEAR(t[i], fn_approx_of_x, expected_error);
    if (std::abs(t[i] - fn_approx_of_x) > expected_error)
      break;
  }
}

TEST(ColorSpace, ApproximateTransferFnBadMatch) {
  const float kStep = 1.f / 512.f;
  ColorSpace::TransferID transfer_ids[3] = {
      ColorSpace::TransferID::IEC61966_2_1, ColorSpace::TransferID::GAMMA22,
      ColorSpace::TransferID::BT709,
  };
  gfx::ColorSpace color_spaces[3];

  // The first iteration will have a perfect match. The second will be very
  // close. The third will converge, but with an error of ~7/256.
  for (size_t transfers_to_use = 1; transfers_to_use <= 3; ++transfers_to_use) {
    std::vector<float> x;
    std::vector<float> t;
    for (size_t c = 0; c < transfers_to_use; ++c) {
      color_spaces[c] =
          gfx::ColorSpace(ColorSpace::PrimaryID::BT709, transfer_ids[c]);
      SkColorSpaceTransferFn tr_fn;
      bool result = color_spaces[c].GetTransferFunction(&tr_fn);
      CHECK(result);

      for (float v = 0; v <= 1.f; v += kStep) {
        x.push_back(v);
        t.push_back(SkTransferFnEval(tr_fn, v));
      }
    }

    SkColorSpaceTransferFn fn_approx;
    bool converged =
        SkApproximateTransferFn(x.data(), t.data(), x.size(), &fn_approx);
    EXPECT_TRUE(converged);

    float expected_error = 1.5f / 256.f;
    if (transfers_to_use == 3)
      expected_error = 8.f / 256.f;

    for (size_t i = 0; i < x.size(); ++i) {
      float fn_approx_of_x = SkTransferFnEval(fn_approx, x[i]);
      EXPECT_NEAR(t[i], fn_approx_of_x, expected_error);
      if (std::abs(t[i] - fn_approx_of_x) > expected_error)
        break;
    }
  }
}

TEST(ColorSpace, ToSkColorSpace) {
  const size_t kNumTests = 4;
  ColorSpace color_spaces[kNumTests] = {
      ColorSpace(ColorSpace::PrimaryID::BT709,
                 ColorSpace::TransferID::IEC61966_2_1),
      ColorSpace(ColorSpace::PrimaryID::ADOBE_RGB,
                 ColorSpace::TransferID::IEC61966_2_1),
      ColorSpace(ColorSpace::PrimaryID::SMPTEST432_1,
                 ColorSpace::TransferID::LINEAR),
      ColorSpace(ColorSpace::PrimaryID::BT2020,
                 ColorSpace::TransferID::IEC61966_2_1),
  };
  sk_sp<SkColorSpace> sk_color_spaces[kNumTests] = {
      SkColorSpace::MakeSRGB(),
      SkColorSpace::MakeRGB(SkColorSpace::kSRGB_RenderTargetGamma,
                            SkColorSpace::kAdobeRGB_Gamut),
      SkColorSpace::MakeRGB(SkColorSpace::kLinear_RenderTargetGamma,
                            SkColorSpace::kDCIP3_D65_Gamut),
      SkColorSpace::MakeRGB(SkColorSpace::kSRGB_RenderTargetGamma,
                            SkColorSpace::kRec2020_Gamut),
  };
  for (size_t i = 0; i < kNumTests; ++i) {
    EXPECT_TRUE(SkColorSpace::Equals(color_spaces[i].ToSkColorSpace().get(),
                                     sk_color_spaces[i].get()))
        << " on iteration i = " << i;
  }
}

}  // namespace
}  // namespace gfx
