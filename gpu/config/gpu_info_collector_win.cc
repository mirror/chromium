// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/config/gpu_info_collector.h"

#include "base/command_line.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/stringprintf.h"
#include "base/trace_event/trace_event.h"
#include "third_party/angle/src/gpu_info_util/SystemInfo.h"
#include "third_party/re2/src/re2/re2.h"
#include "ui/gl/gl_switches.h"

namespace gpu {

#if defined(GOOGLE_CHROME_BUILD) && defined(OFFICIAL_BUILD)
// This function has a real implementation for official builds that can
// be found in src/third_party/amd.
bool GetAMDSwitchableInfo(bool* is_switchable,
                          uint32_t* active_vendor_id,
                          uint32_t* active_device_id);
#else
bool GetAMDSwitchableInfo(bool* is_switchable,
                          uint32_t* active_vendor_id,
                          uint32_t* active_device_id) {
  return false;
}
#endif

namespace {

void CorrectAMDSwitchableInfo(angle::SystemInfo* system_info) {
  bool is_switchable = false;
  uint32_t active_vendor_id = 0;
  uint32_t active_device_id = 0;
  if (!GetAMDSwitchableInfo(&is_switchable, &active_vendor_id,
                            &active_device_id)) {
    return;
  }

  system_info->isAMDSwitchable = is_switchable;

  bool found_active = false;
  for (size_t i = 0; i < system_info->gpus.size(); ++i) {
    if (system_info->gpus[i].vendorId == active_vendor_id &&
        system_info->gpus[i].deviceId == active_device_id) {
      found_active = true;
      system_info->activeGPUIndex = static_cast<int>(i);
    }
  }
  DCHECK(found_active);
}

}  // anonymous namespace

CollectInfoResult CollectContextGraphicsInfo(GPUInfo* gpu_info) {
  TRACE_EVENT0("gpu", "CollectGraphicsInfo");

  DCHECK(gpu_info);

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(switches::kUseGL)) {
    std::string requested_implementation_name =
        base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
            switches::kUseGL);
    if (requested_implementation_name ==
        gl::kGLImplementationSwiftShaderForWebGLName) {
      gpu_info->software_rendering = true;
      gpu_info->context_info_state = kCollectInfoNonFatalFailure;
      return kCollectInfoNonFatalFailure;
    }
  }

  CollectInfoResult result = CollectGraphicsInfoGL(gpu_info);
  if (result != kCollectInfoSuccess) {
    gpu_info->context_info_state = result;
    return result;
  }

  // ANGLE's renderer strings are of the form:
  // ANGLE (<adapter_identifier> Direct3D<version> vs_x_x ps_x_x)
  std::string direct3d_version;
  int vertex_shader_major_version = 0;
  int vertex_shader_minor_version = 0;
  int pixel_shader_major_version = 0;
  int pixel_shader_minor_version = 0;
  if (RE2::FullMatch(gpu_info->gl_renderer,
                     "ANGLE \\(.*\\)") &&
      RE2::PartialMatch(gpu_info->gl_renderer,
                        " Direct3D(\\w+)",
                        &direct3d_version) &&
      RE2::PartialMatch(gpu_info->gl_renderer,
                        " vs_(\\d+)_(\\d+)",
                        &vertex_shader_major_version,
                        &vertex_shader_minor_version) &&
      RE2::PartialMatch(gpu_info->gl_renderer,
                        " ps_(\\d+)_(\\d+)",
                        &pixel_shader_major_version,
                        &pixel_shader_minor_version)) {
    gpu_info->vertex_shader_version =
        base::StringPrintf("%d.%d",
                           vertex_shader_major_version,
                           vertex_shader_minor_version);
    gpu_info->pixel_shader_version =
        base::StringPrintf("%d.%d",
                           pixel_shader_major_version,
                           pixel_shader_minor_version);

    // DirectX diagnostics are collected asynchronously because it takes a
    // couple of seconds.
  } else {
    gpu_info->dx_diagnostics_info_state = kCollectInfoNonFatalFailure;
  }

  gpu_info->context_info_state = kCollectInfoSuccess;
  return kCollectInfoSuccess;
}

CollectInfoResult CollectBasicGraphicsInfo(GPUInfo* gpu_info) {
  TRACE_EVENT0("gpu", "CollectBasicGraphicsInfo");
  DCHECK(gpu_info);

  angle::SystemInfo system_info;
  if (angle::GetSystemInfo(&system_info)) {
    if (system_info.isAMDSwitchable) {
      CorrectAMDSwitchableInfo(&system_info);
    }
    gpu_info->basic_info_state = kCollectInfoSuccess;
    FillGPUInfoFromSystemInfo(gpu_info, &system_info);
  } else if (system_info.primaryDisplayDeviceId == "RDPUDD Chained DD" ||
             system_info.primaryDisplayDeviceId ==
                 "Citrix Systems Inc. Display Driver") {
    gpu_info->basic_info_state = kCollectInfoSuccess;
  } else {
    gpu_info->basic_info_state = kCollectInfoNonFatalFailure;
  }

  return gpu_info->basic_info_state;
}

CollectInfoResult CollectDriverInfoGL(GPUInfo* gpu_info) {
  TRACE_EVENT0("gpu", "CollectDriverInfoGL");

  if (!gpu_info->driver_version.empty())
    return kCollectInfoSuccess;

  bool parsed = RE2::PartialMatch(
      gpu_info->gl_version, "([\\d\\.]+)$", &gpu_info->driver_version);
  return parsed ? kCollectInfoSuccess : kCollectInfoNonFatalFailure;
}

void MergeGPUInfo(GPUInfo* basic_gpu_info,
                  const GPUInfo& context_gpu_info) {
  DCHECK(basic_gpu_info);

  if (context_gpu_info.software_rendering) {
    basic_gpu_info->software_rendering = true;
    return;
  }

  // Track D3D Shader Model (if available)
  const std::string& shader_version =
      context_gpu_info.vertex_shader_version;

  // Only gather if this is the first time we're seeing
  // a non-empty shader version string.
  if (!shader_version.empty() &&
      basic_gpu_info->vertex_shader_version.empty()) {

    // Note: do not reorder, used by UMA_HISTOGRAM below
    enum ShaderModel {
      SHADER_MODEL_UNKNOWN,
      SHADER_MODEL_2_0,
      SHADER_MODEL_3_0,
      SHADER_MODEL_4_0,
      SHADER_MODEL_4_1,
      SHADER_MODEL_5_0,
      NUM_SHADER_MODELS
    };

    ShaderModel shader_model = SHADER_MODEL_UNKNOWN;

    if (shader_version == "5.0") {
      shader_model = SHADER_MODEL_5_0;
    } else if (shader_version == "4.1") {
      shader_model = SHADER_MODEL_4_1;
    } else if (shader_version == "4.0") {
      shader_model = SHADER_MODEL_4_0;
    } else if (shader_version == "3.0") {
      shader_model = SHADER_MODEL_3_0;
    } else if (shader_version == "2.0") {
      shader_model = SHADER_MODEL_2_0;
    }

    UMA_HISTOGRAM_ENUMERATION("GPU.D3DShaderModel",
                              shader_model,
                              NUM_SHADER_MODELS);
  }

  MergeGPUInfoGL(basic_gpu_info, context_gpu_info);

  basic_gpu_info->dx_diagnostics_info_state =
      context_gpu_info.dx_diagnostics_info_state;
  basic_gpu_info->dx_diagnostics = context_gpu_info.dx_diagnostics;
}

}  // namespace gpu
