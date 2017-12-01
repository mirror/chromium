// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/ca_layer_params.h"

namespace gfx {

CALayerParams::CALayerParams() {}
CALayerParams::~CALayerParams() = default;

#if defined(OS_MACOSX) && !defined(OS_IOS)
CALayerParams::CALayerParams(CALayerParams&& params)
    : ca_context_id(params.ca_context_id),
      io_surface_mach_port(std::move(params.io_surface_mach_port)),
      pixel_size(params.pixel_size),
      scale_factor(params.scale_factor) {}

CALayerParams::CALayerParams(const CALayerParams& params)
    : ca_context_id(params.ca_context_id),
      io_surface_mach_port(params.io_surface_mach_port),
      pixel_size(params.pixel_size),
      scale_factor(params.scale_factor) {}

CALayerParams& CALayerParams::operator=(CALayerParams&& params) {
  ca_context_id = params.ca_context_id;
  io_surface_mach_port = std::move(params.io_surface_mach_port);
  pixel_size = params.pixel_size;
  scale_factor = params.scale_factor;
  return *this;
}

CALayerParams& CALayerParams::operator=(const CALayerParams& params) {
  ca_context_id = params.ca_context_id;
  io_surface_mach_port = params.io_surface_mach_port;
  pixel_size = params.pixel_size;
  scale_factor = params.scale_factor;
  return *this;
}
#else
CALayerParams::CALayerParams(CALayerParams&& params) =
    default CALayerParams::CALayerParams(const CALayerParams& params) = default;
CALayerParams& CALayerParams::operator=(CALayerParams&& params) = default;
CALayerParams& CALayerParams::operator=(const CALayerParams& params) = default;
#endif

}  // namespace gfx
