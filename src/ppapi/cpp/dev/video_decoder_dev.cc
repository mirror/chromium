// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/dev/video_decoder_dev.h"

#include "ppapi/c/dev/ppb_video_decoder_dev.h"
#include "ppapi/c/dev/ppp_video_decoder_dev.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/cpp/dev/context_3d_dev.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/module_impl.h"

namespace pp {

namespace {

template <> const char* interface_name<PPB_VideoDecoder_Dev>() {
  return PPB_VIDEODECODER_DEV_INTERFACE;
}

}  // namespace

VideoDecoder_Dev::VideoDecoder_Dev(const Instance& instance) {
  if (!has_interface<PPB_VideoDecoder_Dev>())
    return;
  PassRefFromConstructor(get_interface<PPB_VideoDecoder_Dev>()->Create(
      instance.pp_instance()));
}

VideoDecoder_Dev::VideoDecoder_Dev(PP_Resource resource) : Resource(resource) {
}

VideoDecoder_Dev::~VideoDecoder_Dev() {}

int32_t VideoDecoder_Dev::Initialize(const PP_VideoConfigElement* config,
                                     const Context3D_Dev& context,
                                     CompletionCallback callback) {
  if (!has_interface<PPB_VideoDecoder_Dev>())
    return callback.MayForce(PP_ERROR_NOINTERFACE);
  return get_interface<PPB_VideoDecoder_Dev>()->Initialize(
      pp_resource(), context.pp_resource(), config,
      callback.pp_completion_callback());
}

bool VideoDecoder_Dev::GetConfigs(Instance* instance,
                                  const PP_VideoConfigElement* prototype_config,
                                  PP_VideoConfigElement* matching_configs,
                                  uint32_t matching_configs_size,
                                  uint32_t* num_of_matching_configs) {
  if (!has_interface<PPB_VideoDecoder_Dev>())
    return false;
  return PP_ToBool(get_interface<PPB_VideoDecoder_Dev>()->GetConfigs(
      instance->pp_instance(), prototype_config, matching_configs,
      matching_configs_size, num_of_matching_configs));
}

void VideoDecoder_Dev::AssignGLESBuffers(
    const std::vector<PP_GLESBuffer_Dev>& buffers) {
  if (!has_interface<PPB_VideoDecoder_Dev>() || !pp_resource())
    return;
  get_interface<PPB_VideoDecoder_Dev>()->AssignGLESBuffers(
      pp_resource(), buffers.size(), &buffers[0]);
}

void VideoDecoder_Dev::AssignSysmemBuffers(
    const std::vector<PP_SysmemBuffer_Dev>& buffers) {
  if (!has_interface<PPB_VideoDecoder_Dev>() || !pp_resource())
    return;
  get_interface<PPB_VideoDecoder_Dev>()->AssignSysmemBuffers(
      pp_resource(), buffers.size(), &buffers[0]);
}

int32_t VideoDecoder_Dev::Decode(
    const PP_VideoBitstreamBuffer_Dev& bitstream_buffer,
    CompletionCallback callback) {
  if (!has_interface<PPB_VideoDecoder_Dev>())
    return callback.MayForce(PP_ERROR_NOINTERFACE);
  return get_interface<PPB_VideoDecoder_Dev>()->Decode(
      pp_resource(), &bitstream_buffer, callback.pp_completion_callback());
}

void VideoDecoder_Dev::ReusePictureBuffer(int32_t picture_buffer_id) {
  if (!has_interface<PPB_VideoDecoder_Dev>() || !pp_resource())
    return;
  get_interface<PPB_VideoDecoder_Dev>()->ReusePictureBuffer(
      pp_resource(), picture_buffer_id);
}

int32_t VideoDecoder_Dev::Flush(CompletionCallback callback) {
  if (!has_interface<PPB_VideoDecoder_Dev>())
    return callback.MayForce(PP_ERROR_NOINTERFACE);
  return get_interface<PPB_VideoDecoder_Dev>()->Flush(
      pp_resource(), callback.pp_completion_callback());
}

int32_t VideoDecoder_Dev::Abort(CompletionCallback callback) {
  if (!has_interface<PPB_VideoDecoder_Dev>())
    return callback.MayForce(PP_ERROR_NOINTERFACE);
  return get_interface<PPB_VideoDecoder_Dev>()->Abort(
      pp_resource(), callback.pp_completion_callback());
}

}  // namespace pp
