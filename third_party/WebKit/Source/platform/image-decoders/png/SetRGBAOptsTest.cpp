// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/timer/elapsed_timer.h"
#include "platform/image-decoders/ImageFrame.h"
#include "platform/image-decoders/png/SetRGBA.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class SetRGBAOptsTest : public ::testing::Test {
 public:
  SetRGBAOptsTest() {}
};

// Test data: 32 pixels
unsigned char input[] = {
    255, 16,  25,  45,  150, 230, 0,   121, 0,   98,  5,   65,  150, 176, 25,
    24,  0,   231, 9,   100, 15,  199, 231, 36,  155, 255, 172, 23,  98,  210,
    2,   4,   147, 82,  8,   28,  239, 217, 101, 19,  6,   91,  241, 127, 143,
    190, 133, 231, 168, 24,  44,  156, 122, 230, 85,  96,  174, 189, 36,  76,
    232, 112, 109, 182, 211, 103, 101, 10,  251, 71,  199, 232, 208, 65,  78,
    174, 55,  193, 55,  1,   104, 6,   252, 18,  146, 189, 250, 100, 89,  125,
    192, 199, 106, 156, 66,  2,   180, 135, 4,   39,  1,   181, 203, 69,  216,
    241, 255, 211, 60,  212, 38,  246, 138, 233, 61,  68,  187, 155, 78,  49,
    40,  174, 57,  113, 67,  98,  2,   106};

TEST_F(SetRGBAOptsTest, testCaseAlpha32Pixels) {
  const int kPixels = 32;
  const size_t kRowBytes = kPixels * 4;

  unsigned char output_portable[kRowBytes];
  unsigned char output_optimized[kRowBytes];

  memset(output_portable, 0, kRowBytes);
  memset(output_optimized, 0, kRowBytes);

  unsigned mask_portable = 255;
  unsigned mask_optimized = 255;

  SetRGBARawRow<false>(input, kPixels,
                       (blink::ImageFrame::PixelData*)output_portable,
                       &mask_portable);
  SetRGBARawRow<true>(input, kPixels,
                      (blink::ImageFrame::PixelData*)output_optimized,
                      &mask_optimized);

  EXPECT_EQ(memcmp(output_portable, output_optimized, kRowBytes), 0);
  EXPECT_EQ(mask_portable, mask_optimized);
}

TEST_F(SetRGBAOptsTest, testCaseAlpha21Pixels) {
  const int kPixels = 21;
  const size_t kRowBytes = kPixels * 4;

  unsigned char output_portable[kRowBytes];
  unsigned char output_optimized[kRowBytes];

  memset(output_portable, 0, kRowBytes);
  memset(output_optimized, 0, kRowBytes);

  unsigned mask_portable = 255;
  unsigned mask_optimized = 255;

  SetRGBARawRow<false>(input, kPixels,
                       (blink::ImageFrame::PixelData*)output_portable,
                       &mask_portable);
  SetRGBARawRow<true>(input, kPixels,
                      (blink::ImageFrame::PixelData*)output_optimized,
                      &mask_optimized);

  EXPECT_EQ(memcmp(output_portable, output_optimized, kRowBytes), 0);
  EXPECT_EQ(mask_portable, mask_optimized);
}

TEST_F(SetRGBAOptsTest, testCaseNoAlpha32Pixels) {
  const int kPixels = 32;
  const size_t kRowBytes = kPixels * 4;

  unsigned char output_portable[kRowBytes];
  unsigned char output_optimized[kRowBytes];

  memset(output_portable, 0, kRowBytes);
  memset(output_optimized, 0, kRowBytes);

  SetRGBARawRowNoAlpha<false>(input, kPixels,
                              (blink::ImageFrame::PixelData*)output_portable);
  SetRGBARawRowNoAlpha<true>(input, kPixels,
                             (blink::ImageFrame::PixelData*)output_optimized);

  EXPECT_EQ(memcmp(output_portable, output_optimized, kRowBytes), 0);
}

TEST_F(SetRGBAOptsTest, testCaseNoAlpha21Pixels) {
  const int kPixels = 21;
  const size_t kRowBytes = kPixels * 4;

  unsigned char output_portable[kRowBytes];
  unsigned char output_optimized[kRowBytes];

  memset(output_portable, 0, kRowBytes);
  memset(output_optimized, 0, kRowBytes);

  SetRGBARawRowNoAlpha<false>(input, kPixels,
                              (blink::ImageFrame::PixelData*)output_portable);
  SetRGBARawRowNoAlpha<true>(input, kPixels,
                             (blink::ImageFrame::PixelData*)output_optimized);

  EXPECT_EQ(memcmp(output_portable, output_optimized, kRowBytes), 0);
}

#if (defined(__ARM_NEON) || defined(__ARM_NEON__))
template <bool optimized>
static base::TimeDelta testNpixels(const size_t kPixels, bool alpha) {
  const size_t input_len = alpha ? kPixels * 4 : kPixels * 3;
  unsigned char* input = new unsigned char[input_len];
  unsigned char* output = new unsigned char[kPixels * 4];

  auto cleanup = [&]() {
    if (input)
      delete[] input;
    if (output)
      delete[] output;
  };

  if (!input || !output) {
    cleanup();
    ADD_FAILURE();
    return base::TimeDelta();
  }

  base::TimeDelta result;
  {
    base::ElapsedTimer runTime;
    if (alpha) {
      unsigned alpha_mask = 255;
      SetRGBARawRow<optimized>(
          input, kPixels, (blink::ImageFrame::PixelData*)output, &alpha_mask);
    } else {
      SetRGBARawRowNoAlpha<optimized>(input, kPixels,
                                      (blink::ImageFrame::PixelData*)output);
    }
    result = runTime.Elapsed();
  }

  cleanup();
  return result;
}

TEST_F(SetRGBAOptsTest, testPerf1kAlpha) {
  auto optimized_elapsed = testNpixels<true>(1000, true);
  auto portable_elapsed = testNpixels<false>(1000, true);

  EXPECT_TRUE(optimized_elapsed < portable_elapsed)
      << "Optimized: " << optimized_elapsed
      << "\tPortable: " << portable_elapsed << std::endl;
}

TEST_F(SetRGBAOptsTest, testPerf4kAlpha) {
  auto optimized_elapsed = testNpixels<true>(4000, true);
  auto portable_elapsed = testNpixels<false>(4000, true);

  EXPECT_TRUE(optimized_elapsed < portable_elapsed)
      << "Optimized: " << optimized_elapsed
      << "\tPortable: " << portable_elapsed << std::endl;
}

TEST_F(SetRGBAOptsTest, testPerf1kNoAlpha) {
  auto optimized_elapsed = testNpixels<true>(1000, false);
  auto portable_elapsed = testNpixels<false>(1000, false);

  EXPECT_TRUE(optimized_elapsed < portable_elapsed)
      << "Optimized: " << optimized_elapsed
      << "\tPortable: " << portable_elapsed << std::endl;
}

TEST_F(SetRGBAOptsTest, testPerf4kNoAlpha) {
  auto optimized_elapsed = testNpixels<true>(4000, false);
  auto portable_elapsed = testNpixels<false>(4000, false);

  EXPECT_TRUE(optimized_elapsed < portable_elapsed)
      << "Optimized: " << optimized_elapsed
      << "\tPortable: " << portable_elapsed << std::endl;
}
#endif

};  // namespace blink
