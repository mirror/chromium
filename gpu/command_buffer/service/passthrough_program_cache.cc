// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/passthrough_program_cache.h"

#include <stddef.h>

#include "ui/gl/angle_platform_impl.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_surface_egl.h"

#ifndef EGL_ANGLE_program_cache_control
#define EGL_ANGLE_program_cache_control 1
#define EGL_PROGRAM_CACHE_SIZE_ANGLE 0x3455
#define EGL_PROGRAM_CACHE_KEY_LENGTH_ANGLE 0x3456
#define EGL_PROGRAM_CACHE_RESIZE_ANGLE 0x3457
#define EGL_PROGRAM_CACHE_TRIM_ANGLE 0x3458
#define EGL_CONTEXT_PROGRAM_BINARY_CACHE_ENABLED_ANGLE 0x3459
#endif /* EGL_ANGLE_program_cache_control */

namespace gpu {
namespace gles2 {

namespace {

bool ProgramCacheControlExtensionAvailable() {
  // The display should be initialized if the extension is available.
  return gl::g_driver_egl.ext.b_EGL_ANGLE_program_cache_control;
}

}  // namespace

PassthroughProgramCache::PassthroughProgramCache(
    size_t max_cache_size_bytes,
    bool disable_gpu_shader_disk_cache)
    : max_size_bytes_(max_cache_size_bytes),
      disable_gpu_shader_disk_cache_(disable_gpu_shader_disk_cache) {
  if (!ProgramCacheControlExtensionAvailable()) {
    return;
  }

  EGLDisplay display = gl::GLSurfaceEGL::GetHardwareDisplay();
  DCHECK(display != EGL_NO_DISPLAY);

  eglProgramCacheResizeANGLE(display, max_cache_size_bytes,
                             EGL_PROGRAM_CACHE_RESIZE_ANGLE);
}

PassthroughProgramCache::~PassthroughProgramCache() {
  // Ensure the program cache callback is cleared.
  angle::ResetCacheProgramCallback();
}

void PassthroughProgramCache::ClearBackend() {
  Trim(0);
}

ProgramCache::ProgramLoadResult PassthroughProgramCache::LoadLinkedProgram(
    GLuint program,
    Shader* shader_a,
    Shader* shader_b,
    const LocationMap* bind_attrib_location_map,
    const std::vector<std::string>& transform_feedback_varyings,
    GLenum transform_feedback_buffer_mode,
    GLES2DecoderClient* client) {
  // We no-op this call with the cache control extension.
  return PROGRAM_LOAD_FAILURE;
}

void PassthroughProgramCache::SaveLinkedProgram(
    GLuint program,
    const Shader* shader_a,
    const Shader* shader_b,
    const LocationMap* bind_attrib_location_map,
    const std::vector<std::string>& transform_feedback_varyings,
    GLenum transform_feedback_buffer_mode,
    GLES2DecoderClient* client) {
  // We no-op this call with the cache control extension.
}

void PassthroughProgramCache::LoadProgram(const std::string& key,
                                          const std::string& program) {
  if (!ProgramCacheControlExtensionAvailable()) {
    // Early exit if this display can't support cache control
    return;
  }

  EGLDisplay display = gl::GLSurfaceEGL::GetHardwareDisplay();
  DCHECK(display != EGL_NO_DISPLAY);

  eglProgramCachePopulateANGLE(display, key.c_str(), key.size(),
                               program.c_str(), program.size());
}

size_t PassthroughProgramCache::Trim(size_t limit) {
  if (!ProgramCacheControlExtensionAvailable()) {
    // Early exit if this display can't support cache control
    return 0;
  }

  EGLDisplay display = gl::GLSurfaceEGL::GetHardwareDisplay();
  DCHECK(display != EGL_NO_DISPLAY);

  EGLint trimmed =
      eglProgramCacheResizeANGLE(display, limit, EGL_PROGRAM_CACHE_TRIM_ANGLE);
  return static_cast<size_t>(trimmed);
}

}  // namespace gles2
}  // namespace gpu
