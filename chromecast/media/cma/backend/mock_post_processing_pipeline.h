// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_CMA_BACKEND_MOCK_POST_PROCESSING_PIPELINE_H_
#define CHROMECAST_MEDIA_CMA_BACKEND_MOCK_POST_PROCESSING_PIPELINE_H_

#include <unordered_map>

#include "base/macros.h"
#include "chromecast/media/cma/backend/post_processing_pipeline.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace chromecast {
namespace media {

class MockPostProcessingPipeline : public PostProcessingPipeline {
 public:
  MockPostProcessingPipeline(const std::string& name,
                             const base::ListValue* filter_description_list,
                             int channels);

  ~MockPostProcessingPipeline() override;

  // PostProcessingPipeline implementation.
  MOCK_METHOD4(
      ProcessFrames,
      int(float* data, int num_frames, float current_volume, bool is_silence));
  bool SetSampleRate(int sample_rate) override;
  bool IsRinging() override;
  MOCK_METHOD2(SetPostProcessorConfig,
               void(const std::string& name, const std::string& config));

  int delay() { return rendering_delay_; }
  std::string name() const { return name_; }

  static std::unordered_map<std::string, MockPostProcessingPipeline*>*
  instances();

 private:
  int DoProcessFrames(float* data,
                      int num_frames,
                      float current_volume,
                      bool is_sience);

  std::string name_;
  int rendering_delay_ = 0;
  bool ringing_ = false;

  DISALLOW_COPY_AND_ASSIGN(MockPostProcessingPipeline);
};

class MockPostProcessingPipelineFactory : public PostProcessingPipelineFactory {
 public:
  MockPostProcessingPipelineFactory();
  ~MockPostProcessingPipelineFactory() override;

  std::unique_ptr<PostProcessingPipeline> CreatePipeline(
      const std::string& name,
      const base::ListValue* filter_description_list,
      int channels) override;
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_CMA_BACKEND_MOCK_POST_PROCESSING_PIPELINE_H_
