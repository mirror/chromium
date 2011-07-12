// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/thunk/thunk.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_context_3d_api.h"
#include "ppapi/thunk/resource_creation_api.h"

namespace ppapi {
namespace thunk {

namespace {

typedef EnterResource<PPB_Context3D_API> EnterContext3D;

void* MapTexSubImage2DCHROMIUM(PP_Resource context,
                               GLenum target,
                               GLint level,
                               GLint xoffset,
                               GLint yoffset,
                               GLsizei width,
                               GLsizei height,
                               GLenum format,
                               GLenum type,
                               GLenum access) {
  EnterContext3D enter(context, true);
  if (enter.failed())
    return NULL;
  return enter.object()->MapTexSubImage2DCHROMIUM(
      target, level, xoffset, yoffset, width, height, format, type, access);
}

void UnmapTexSubImage2DCHROMIUM(PP_Resource context, const void* mem) {
  EnterContext3D enter(context, true);
  if (enter.succeeded())
    enter.object()->UnmapTexSubImage2DCHROMIUM(mem);
}

const PPB_GLESChromiumTextureMapping_Dev
g_ppb_gles_chromium_texture_mapping_thunk = {
  &MapTexSubImage2DCHROMIUM,
  &UnmapTexSubImage2DCHROMIUM
};

}  // namespace

const PPB_GLESChromiumTextureMapping_Dev*
GetPPB_GLESChromiumTextureMapping_Thunk() {
  return &g_ppb_gles_chromium_texture_mapping_thunk;
}

}  // namespace thunk
}  // namespace ppapi
