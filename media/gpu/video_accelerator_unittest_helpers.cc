// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "media/gpu/video_accelerator_unittest_helpers.h"

namespace media {
namespace {
void vp8_ssim_parms_8x8_c(const unsigned char* s,
                          int sp,
                          const unsigned char* r,
                          int rp,
                          uint32_t* sum_s,
                          uint32_t* sum_r,
                          uint32_t* sum_sq_s,
                          uint32_t* sum_sq_r,
                          uint32_t* sum_sxr) {
  int i, j;
  for (i = 0; i < 8; i++, s += sp, r += rp) {
    for (j = 0; j < 8; j++) {
      *sum_s += s[j];
      *sum_r += r[j];
      *sum_sq_s += s[j] * s[j];
      *sum_sq_r += r[j] * r[j];
      *sum_sxr += s[j] * r[j];
    }
  }
}

static const int64_t cc1 = 26634;   // (64^2*(.01*255)^2
static const int64_t cc2 = 239708;  // (64^2*(.03*255)^2

static double similarity(uint32_t sum_s,
                         uint32_t sum_r,
                         uint32_t sum_sq_s,
                         uint32_t sum_sq_r,
                         uint32_t sum_sxr,
                         int count) {
  int64_t ssim_n, ssim_d;
  int64_t c1, c2;

  // scale the constants by number of pixels
  c1 = (cc1 * count * count) >> 12;
  c2 = (cc2 * count * count) >> 12;

  ssim_n = (2 * sum_s * sum_r + c1) *
           ((int64_t)2 * count * sum_sxr - (int64_t)2 * sum_s * sum_r + c2);

  ssim_d = (sum_s * sum_s + sum_r * sum_r + c1) *
           ((int64_t)count * sum_sq_s - (int64_t)sum_s * sum_s +
            (int64_t)count * sum_sq_r - (int64_t)sum_r * sum_r + c2);

  return ssim_n * 1.0 / ssim_d;
}

static double ssim_8x8(const unsigned char* s,
                       int sp,
                       const unsigned char* r,
                       int rp) {
  uint32_t sum_s = 0, sum_r = 0, sum_sq_s = 0, sum_sq_r = 0, sum_sxr = 0;
  vp8_ssim_parms_8x8_c(s, sp, r, rp, &sum_s, &sum_r, &sum_sq_s, &sum_sq_r,
                       &sum_sxr);
  return similarity(sum_s, sum_r, sum_sq_s, sum_sq_r, sum_sxr, 64);
}

// We are using a 8x8 moving window with starting location of each 8x8 window
// on the 4x4 pixel grid. Such arrangement allows the windows to overlap
// block boundaries to penalize blocking artifacts.
double vp8_ssim2(const unsigned char* img1,
                 const unsigned char* img2,
                 int stride_img1,
                 int stride_img2,
                 int width,
                 int height) {
  int i, j;
  int samples = 0;
  double ssim_total = 0;

  // sample point start with each 4x4 location
  for (i = 0; i <= height - 8;
       i += 4, img1 += stride_img1 * 4, img2 += stride_img2 * 4) {
    for (j = 0; j <= width - 8; j += 4) {
      double v = ssim_8x8(img1 + j, stride_img1, img2 + j, stride_img2);
      ssim_total += v;
      samples++;
    }
  }
  ssim_total /= samples;
  return ssim_total;
}

static uint64_t calc_plane_error(const uint8_t* orig,
                                 int orig_stride,
                                 const uint8_t* recon,
                                 int recon_stride,
                                 unsigned int cols,
                                 unsigned int rows) {
  unsigned int row, col;
  uint64_t total_sse = 0;
  int diff;

  for (row = 0; row < rows; row++) {
    for (col = 0; col < cols; col++) {
      diff = orig[col] - recon[col];
      total_sse += diff * diff;
    }

    orig += orig_stride;
    recon += recon_stride;
  }

  return total_sse;
}

void GenerateMseAndSsim(double* ssim,
                        uint64_t* mse,
                        const unsigned char* buf0,
                        const unsigned char* buf1,
                        int w,
                        int h,
                        size_t stride0,
                        size_t stride1) {
  *ssim = vp8_ssim2(buf0, buf1, stride0, stride1, w, h);
  *mse = calc_plane_error(buf0, stride0, buf1, stride1, w, h);
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
                     frame_stats.height,
                     original_frame.stride(VideoFrame::kYPlane),
                     output_frame.stride(VideoFrame::kYPlane));
  GenerateMseAndSsim(&frame_stats.ssim_u, &frame_stats.mse_u,
                     original_frame.data(VideoFrame::kUPlane),
                     output_frame.data(VideoFrame::kUPlane),
                     frame_stats.width / 2, frame_stats.height / 2,
                     original_frame.stride(VideoFrame::kUPlane),
                     output_frame.stride(VideoFrame::kUPlane));
  GenerateMseAndSsim(&frame_stats.ssim_v, &frame_stats.mse_v,
                     original_frame.data(VideoFrame::kVPlane),
                     output_frame.data(VideoFrame::kVPlane),
                     frame_stats.width / 2, frame_stats.height / 2,
                     original_frame.stride(VideoFrame::kVPlane),
                     output_frame.stride(VideoFrame::kVPlane));
  return frame_stats;
}

}  // namespace media
