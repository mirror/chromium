// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SetRGBA_h
#define SetRGBA_h

#include "platform/image-decoders/ImageFrame.h"
#include "png.h"

template <bool optimized>
inline void SetRGBARawRow(png_bytep src_ptr,
                          const int pixel_count,
                          blink::ImageFrame::PixelData* dst_pixel,
                          unsigned* const alpha_mask) {
  assert(dst_pixel);
  assert(alpha_mask);
  for (int i = 0; i < pixel_count; i++, src_ptr += 4, dst_pixel++) {
    blink::ImageFrame::SetRGBARaw(dst_pixel, src_ptr[0], src_ptr[1], src_ptr[2],
                                  src_ptr[3]);
    *alpha_mask &= src_ptr[3];
  }
}

template <bool optimized>
inline void SetRGBARawRowNoAlpha(png_bytep src_ptr,
                                 const int pixel_count,
                                 blink::ImageFrame::PixelData* dst_pixel) {
  assert(dst_pixel);
  for (int i = 0; i < pixel_count; i++, src_ptr += 3, dst_pixel++) {
    blink::ImageFrame::SetRGBARaw(dst_pixel, src_ptr[0], src_ptr[1], src_ptr[2],
                                  255);
  }
}

#if (defined(__ARM_NEON) || defined(__ARM_NEON__))
#include <arm_neon.h>

template <>
inline void SetRGBARawRow<true>(png_bytep src_ptr,
                                const int pixel_count,
                                blink::ImageFrame::PixelData* dst_pixel,
                                unsigned* const alpha_mask) {
  assert(dst_pixel);
  assert(alpha_mask);

  constexpr int kPixelsPerLoad = 16;
  constexpr int kRgbaStep = kPixelsPerLoad * 4;
  // Input registers.
  uint8x16x4_t rgba;
  // Output registers.
  uint8x16x4_t bgra;
  // Alpha mask.
  uint8x16_t alpha_mask_vector = vdupq_n_u8(255);

  int i = pixel_count;
  for (; i >= kPixelsPerLoad; i -= kPixelsPerLoad) {
    // Reads 16 pixels at once, each color channel in a different
    // 128-bit register.
    rgba = vld4q_u8(src_ptr);

    // Re-order color channels for little endian.
    bgra.val[0] = rgba.val[2];
    bgra.val[1] = rgba.val[1];
    bgra.val[2] = rgba.val[0];
    bgra.val[3] = rgba.val[3];

    // AND pixel alpha values with the alpha detection mask.
    alpha_mask_vector = vandq_u8(alpha_mask_vector, rgba.val[3]);

    // Write back (interleaved) results to memory.
    vst4q_u8(reinterpret_cast<uint8_t*>(dst_pixel), bgra);

    // Advance to next elements.
    src_ptr += kRgbaStep;
    dst_pixel += kPixelsPerLoad;
  }

  // AND together the 16 alpha values in the alpha_mask_vector.
  uint64_t alpha_mask_u64 =
      vget_lane_u64(vreinterpret_u64_u8(vget_low_u8(alpha_mask_vector)), 0);
  alpha_mask_u64 &=
      vget_lane_u64(vreinterpret_u64_u8(vget_high_u8(alpha_mask_vector)), 0);
  alpha_mask_u64 &= (alpha_mask_u64 >> 32);
  alpha_mask_u64 &= (alpha_mask_u64 >> 16);
  alpha_mask_u64 &= (alpha_mask_u64 >> 8);
  *alpha_mask &= alpha_mask_u64;

  // Handle the tail elements.
  for (; i > 0; i--, dst_pixel++, src_ptr += 4) {
    blink::ImageFrame::SetRGBARaw(dst_pixel, src_ptr[0], src_ptr[1], src_ptr[2],
                                  src_ptr[3]);
    *alpha_mask &= src_ptr[3];
  }
}

template <>
inline void SetRGBARawRowNoAlpha<true>(
    png_bytep src_ptr,
    const int pixel_count,
    blink::ImageFrame::PixelData* dst_pixel) {
  assert(dst_pixel);

  constexpr int kPixelsPerLoad = 16;
  constexpr int kRgbStep = kPixelsPerLoad * 3;
  // Input registers.
  uint8x16x3_t rgb;
  // Output registers.
  uint8x16x4_t bgra;

  int i = pixel_count;
  for (; i >= kPixelsPerLoad; i -= kPixelsPerLoad) {
    // Reads 16 pixels at once, each color channel in a different
    // 128-bit register.
    rgb = vld3q_u8(src_ptr);

    // Re-order color channels for little endian.
    bgra.val[0] = rgb.val[2];
    bgra.val[1] = rgb.val[1];
    bgra.val[2] = rgb.val[0];
    bgra.val[3] = vdupq_n_u8(255);

    // Write back (interleaved) results to memory.
    vst4q_u8(reinterpret_cast<uint8_t*>(dst_pixel), bgra);

    // Advance to next elements.
    src_ptr += kRgbStep;
    dst_pixel += kPixelsPerLoad;
  }

  // Handle the tail elements.
  for (; i > 0; i--, dst_pixel++, src_ptr += 3) {
    blink::ImageFrame::SetRGBARaw(dst_pixel, src_ptr[0], src_ptr[1], src_ptr[2],
                                  255);
  }
}
#endif

#endif