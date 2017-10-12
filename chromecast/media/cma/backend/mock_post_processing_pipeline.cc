// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/mock_post_processing_pipeline.h"

#include <memory>

#include "base/logging.h"
#include "base/values.h"

using testing::_;

namespace chromecast {
namespace media {

namespace {
std::unordered_map<std::string, MockPostProcessingPipeline*>
    g_mock_post_processor_instances;
}  // namespace

MockPostProcessingPipelineFactory::MockPostProcessingPipelineFactory() =
    default;
MockPostProcessingPipelineFactory::~MockPostProcessingPipelineFactory() =
    default;

std::unique_ptr<PostProcessingPipeline>
MockPostProcessingPipelineFactory::CreatePipeline(
    const std::string& name,
    const base::ListValue* filter_description_list,
    int channels) {
  return std::make_unique<testing::NiceMock<MockPostProcessingPipeline>>(
      name, filter_description_list, channels);
}

MockPostProcessingPipeline::MockPostProcessingPipeline(
    const std::string& name,
    const base::ListValue* filter_description_list,
    int channels)
    : name_(name) {
  CHECK(g_mock_post_processor_instances.insert({name_, this}).second);

  if (!filter_description_list) {
    // This happens for PostProcessingPipeline with no post-processors.
    return;
  }

  // Parse |filter_description_list| for parameters.
  for (size_t i = 0; i < filter_description_list->GetSize(); ++i) {
    const base::DictionaryValue* description_dict;
    CHECK(filter_description_list->GetDictionary(i, &description_dict));
    std::string solib;
    CHECK(description_dict->GetString("processor", &solib));
    // This will initially be called with the actual pipeline on creation.
    // Ignore and wait for the call to ResetPostProcessorsForTest.
    if (solib == kDelayModuleSolib) {
      const base::DictionaryValue* processor_config_dict;
      CHECK(description_dict->GetDictionary("config", &processor_config_dict));
      int module_delay;
      CHECK(processor_config_dict->GetInteger("delay", &module_delay));
      rendering_delay_ += module_delay;
      processor_config_dict->GetBoolean("ringing", &ringing_);
    }
  }
  ON_CALL(*this, ProcessFrames(_, _, _, _))
      .WillByDefault(
          testing::Invoke(this, &MockPostProcessingPipeline::DoProcessFrames));
}

MockPostProcessingPipeline ~MockPostProcessingPipeline() override {
  g_mock_post_processor_instances.erase(name_);
}

bool MockPostProcessingPipeline::SetSampleRate(int sample_rate) override {
  return false;
}

bool MockPostProcessingPipeline::IsRinging() override {
  return ringing_;
}

// static
std::unordered_map<std::string, MockPostProcessingPipeline*>*
MockPostProcessingPipeline::instances() {
  return &g_mock_post_processor_instances;
}

int MockPostProcessingPipeline::DoProcessFrames(float* data,
                                                int num_frames,
                                                float current_volume,
                                                bool is_sience) {
  return rendering_delay_;
}

}  // namespace media
}  // namespace chromecast
