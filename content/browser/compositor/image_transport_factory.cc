// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/compositor/image_transport_factory.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "ui/compositor/compositor_switches.h"
#include "ui/gl/gl_implementation.h"

namespace content {
namespace {

ImageTransportFactory* g_factory = nullptr;
gl::DisableNullDrawGLBindings* g_disable_null_draw = nullptr;

}  // namespace

// static
void ImageTransportFactory::SetFactory(
    std::unique_ptr<ImageTransportFactory> factory) {
  DCHECK(!g_factory);
  g_factory = factory.release();
}

// static
void ImageTransportFactory::SetFactoryForTests(
    std::unique_ptr<ImageTransportFactory> factory) {
  SetFactory(std::move(factory));

  const auto* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kEnablePixelOutputInTests))
    g_disable_null_draw = new gl::DisableNullDrawGLBindings;
}

// static
void ImageTransportFactory::Terminate() {
  delete g_factory;
  g_factory = nullptr;
  delete g_disable_null_draw;
  g_disable_null_draw = nullptr;
}

// static
ImageTransportFactory* ImageTransportFactory::GetInstance() {
  return g_factory;
}

}  // namespace content
