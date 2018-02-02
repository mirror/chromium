// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated from the Jinja2 template
// third_party/WebKit/Source/bindings/templates/dictionary_impl.cpp.tmpl
// by the script code_generator_v8.py.
// DO NOT MODIFY!

// clang-format off
#include "CanvasContextCreationAttributes.h"

namespace blink {

CanvasContextCreationAttributes::CanvasContextCreationAttributes():
  alpha_(true),
  antialias_(true),
  color_space_("srgb"),
  depth_(true),
  fail_if_major_performance_caveat_(false),
  low_latency_(false),
  pixel_format_("8-8-8-8"),
  premultiplied_alpha_(true),
  preserve_drawing_buffer_(false),
  stencil_(false) {}

CanvasContextCreationAttributes::CanvasContextCreationAttributes(const CanvasContextCreationAttributes& other) {
  setAlpha(other.alpha());
  setDepth(other.depth());
  setStencil(other.stencil());
  setAntialias(other.antialias());
  setPremultipliedAlpha(other.premultipliedAlpha());
  setPreserveDrawingBuffer(other.preserveDrawingBuffer());
  setFailIfMajorPerformanceCaveat(other.failIfMajorPerformanceCaveat());
  setCompatibleXRDevice(other.compatibleXRDevice());
}

CanvasContextCreationAttributes::~CanvasContextCreationAttributes() {}

void CanvasContextCreationAttributes::setAlpha(bool value) {
  alpha_ = value;
  has_alpha_ = true;
}

void CanvasContextCreationAttributes::setAntialias(bool value) {
  antialias_ = value;
  has_antialias_ = true;
}

void CanvasContextCreationAttributes::setColorSpace(const String& value) {
  color_space_ = value;
}

void CanvasContextCreationAttributes::setDepth(bool value) {
  depth_ = value;
  has_depth_ = true;
}

void CanvasContextCreationAttributes::setFailIfMajorPerformanceCaveat(bool value) {
  fail_if_major_performance_caveat_ = value;
  has_fail_if_major_performance_caveat_ = true;
}

void CanvasContextCreationAttributes::setLowLatency(bool value) {
  low_latency_ = value;
  has_low_latency_ = true;
}

void CanvasContextCreationAttributes::setPixelFormat(const String& value) {
  pixel_format_ = value;
}

void CanvasContextCreationAttributes::setPremultipliedAlpha(bool value) {
  premultiplied_alpha_ = value;
  has_premultiplied_alpha_ = true;
}

void CanvasContextCreationAttributes::setPreserveDrawingBuffer(bool value) {
  preserve_drawing_buffer_ = value;
  has_preserve_drawing_buffer_ = true;
}

void CanvasContextCreationAttributes::setStencil(bool value) {
  stencil_ = value;
  has_stencil_ = true;
}

void CanvasContextCreationAttributes::setCompatibleXRDevice(EventTargetWithInlineData* value) {
  compatiblexr_device_ = value;
}

void CanvasContextCreationAttributes::Trace(blink::Visitor* visitor) {
  visitor->Trace(compatiblexr_device_);
}

}  // namespace blink
