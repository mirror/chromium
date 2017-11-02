// copyright 2017 the chromium authors. all rights reserved.
// use of this source code is governed by a bsd-style license that can be
// found in the license file.

#include "remoting/base/descending_samples.h"

namespace remoting {

// Receives bandwidth estimations, frame size, etc and decide the best bitrate
// for encoder.
class EncoderBitrateFilter final {
 public:
  explicit EncoderBitrateFilter(int minimum_bitrate_kbps_per_megapixel);
  ~EncoderBitrateFilter();

  void SetBandwidthEstimateKbps(int bandwidth_kbps);
  void SetFrameSize(int width, int height);
  int GetTargetBitrateKbps() const;

 private:
  const int64_t minimum_bitrate_kbps_per_megapixel_;
  // This is the minimum number to avoid returning unreasonable value from
  // GetTargetBitrateKbps(). It roughly equals to the minimum bitrate of a 780 x
  // 512 screen for VP8, or 1024 x 558 screen for H264.
  int minimum_bitrate_kbps_ = 1000;
  DescendingSamples bandwidth_kbps_;
  int bitrate_kbps_ = minimum_bitrate_kbps_;
};

}  // namespace remoting
