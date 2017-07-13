// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "media/gpu/video_accelerator_unittest_helpers.h"

#include <stdint.h>

namespace media {
namespace {
void GetSsimParams8x8(const uint8_t* s,
                      const uint8_t* r,
                      int width,
                      uint32_t* sum_s,
                      uint32_t* sum_r,
                      uint32_t* sum_sq_s,
                      uint32_t* sum_sq_r,
                      uint32_t* sum_sxr) {
  for (size_t i = 0; i < 8; i++, s += width, r += width) {
    for (size_t j = 0; j < 8; j++) {
      *sum_s += s[j];
      *sum_r += r[j];
      *sum_sq_s += s[j] * s[j];
      *sum_sq_r += r[j] * r[j];
      *sum_sxr += s[j] * r[j];
    }
  }
}

static double GenerateSimilarity(uint32_t sum_s,
                                 uint32_t sum_r,
                                 uint32_t sum_sq_s,
                                 uint32_t sum_sq_r,
                                 uint32_t sum_sxr,
                                 int count) {
  static const int64_t kC1 = 26634;   // (64^2*(.01*255)^2
  static const int64_t kC2 = 239708;  // (64^2*(.03*255)^2

  // scale the constants by number of pixels
  int64_t c1 = (kC1 * count * count) >> 12;
  int64_t c2 = (kC2 * count * count) >> 12;

  int64_t ssim_n = (2 * sum_s * sum_r + c1) * ((int64_t)2 * count * sum_sxr -
                                               (int64_t)2 * sum_s * sum_r + c2);

  int64_t ssim_d = (sum_s * sum_s + sum_r * sum_r + c1) *
                   ((int64_t)count * sum_sq_s - (int64_t)sum_s * sum_s +
                    (int64_t)count * sum_sq_r - (int64_t)sum_r * sum_r + c2);

  return ssim_n * 1.0 / ssim_d;
}

static double GenerateSsim8x8(const uint8_t* s, const uint8_t* r, int width) {
  uint32_t sum_s = 0, sum_r = 0, sum_sq_s = 0, sum_sq_r = 0, sum_sxr = 0;
  GetSsimParams8x8(s, r, width, &sum_s, &sum_r, &sum_sq_s, &sum_sq_r, &sum_sxr);
  return GenerateSimilarity(sum_s, sum_r, sum_sq_s, sum_sq_r, sum_sxr, 64);
}

// We are using a 8x8 moving window with starting location of each 8x8 window
// on the 4x4 pixel grid. Such arrangement allows the windows to overlap
// block boundaries to penalize blocking artifacts.
double GenerateSsim(const uint8_t* img1,
                    const uint8_t* img2,
                    int width,
                    int height) {
  int i, j;
  int samples = 0;
  double ssim_total = 0;

  // sample point start with each 4x4 location
  for (i = 0; i <= height - 8; i += 4, img1 += width * 4, img2 += width * 4) {
    for (j = 0; j <= width - 8; j += 4) {
      double v = GenerateSsim8x8(img1 + j, img2 + j, width);
      ssim_total += v;
      samples++;
    }
  }
  ssim_total /= samples;
  return ssim_total;
}

static uint64_t GenerateMse(const uint8_t* orig,
                            const uint8_t* recon,
                            int width,
                            int height) {
  uint64_t total_sse = 0;
  for (int row = 0; row < height; ++row) {
    for (int col = 0; col < width; ++col) {
      int diff = orig[col] - recon[col];
      total_sse += diff * diff;
    }

    orig += width;
    recon += width;
  }

  return total_sse;
}

void GenerateMseAndSsim(double* ssim,
                        uint64_t* mse,
                        const uint8_t* buf0,
                        const uint8_t* buf1,
                        int w,
                        int h) {
  *ssim = GenerateSsim(buf0, buf1, w, h);
  *mse = GenerateMse(buf0, buf1, w, h);
}

}  // namespace

FrameStats FrameStats::CompareFrames(const VideoFrame& original_frame,
                                     const VideoFrame& output_frame) {
  FrameStats frame_stats;
  gfx::Size visible_size = original_frame.visible_rect().size();
  frame_stats.width = visible_size.width();
  frame_stats.height = visible_size.height();
  GenerateMseAndSsim(&frame_stats.ssim_y, &frame_stats.mse_y,
                     original_frame.data(VideoFrame::kYPlane),
                     output_frame.data(VideoFrame::kYPlane), frame_stats.width,
                     frame_stats.height);
  GenerateMseAndSsim(&frame_stats.ssim_u, &frame_stats.mse_u,
                     original_frame.data(VideoFrame::kUPlane),
                     output_frame.data(VideoFrame::kUPlane),
                     frame_stats.width / 2, frame_stats.height / 2);
  GenerateMseAndSsim(&frame_stats.ssim_v, &frame_stats.mse_v,
                     original_frame.data(VideoFrame::kVPlane),
                     output_frame.data(VideoFrame::kVPlane),
                     frame_stats.width / 2, frame_stats.height / 2);
  return frame_stats;
}

}  // namespace media
