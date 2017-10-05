// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains Chromium-specific EGL extensions declarations.

#ifndef UI_GL_EGL_EGLEXTCHROMIUM_H_
#define UI_GL_EGL_EGLEXTCHROMIUM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <EGL/eglplatform.h>

/* EGLSyncControlCHROMIUM requires 64-bit uint support */
#if KHRONOS_SUPPORT_INT64
#ifndef EGL_CHROMIUM_sync_control
#define EGL_CHROMIUM_sync_control 1
typedef khronos_uint64_t EGLuint64CHROMIUM;
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLBoolean EGLAPIENTRY eglGetSyncValuesCHROMIUM(
    EGLDisplay dpy, EGLSurface surface, EGLuint64CHROMIUM *ust,
    EGLuint64CHROMIUM *msc, EGLuint64CHROMIUM *sbc);
#endif /* EGL_EGLEXT_PROTOTYPES */
typedef EGLBoolean (EGLAPIENTRYP PFNEGLGETSYNCVALUESCHROMIUMPROC)
    (EGLDisplay dpy, EGLSurface surface, EGLuint64CHROMIUM *ust,
     EGLuint64CHROMIUM *msc, EGLuint64CHROMIUM *sbc);
#endif
#endif

/* Chromium-specific support for EGL_EXT_image_flush_external extension */
#ifndef EGL_EXT_image_flush_external
#define EGL_EXT_image_flush_external 1
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLBoolean EGLAPIENTRY
eglImageFlushExternalEXT(EGLDisplay dpy,
                         EGLImageKHR image,
                         const EGLint* attrib_list);
#else
typedef EGLBoolean(EGLAPIENTRYP PFNGLEGLIMAGEFLUSHEXTERNALEXT)(
    EGLDisplay dpy,
    EGLImageKHR image,
    const EGLint* attrib_list);
#endif
#endif

#ifndef EGL_ANGLE_stream_producer_d3d_texture_nv12
#define EGL_ANGLE_stream_producer_d3d_texture_nv12
#define EGL_D3D_TEXTURE_SUBRESOURCE_ID_ANGLE 0x33AB
typedef EGLBoolean(
    EGLAPIENTRYP PFNEGLCREATESTREAMPRODUCERD3DTEXTURENV12ANGLEPROC)(
    EGLDisplay dpy,
    EGLStreamKHR stream,
    const EGLAttrib* attrib_list);
typedef EGLBoolean(EGLAPIENTRYP PFNEGLSTREAMPOSTD3DTEXTURENV12ANGLEPROC)(
    EGLDisplay dpy,
    EGLStreamKHR stream,
    void* texture,
    const EGLAttrib* attrib_list);
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLBoolean EGLAPIENTRY
eglCreateStreamProducerD3DTextureNV12ANGLE(EGLDisplay dpy,
                                           EGLStreamKHR stream,
                                           const EGLAttrib* attrib_list);
EGLAPI EGLBoolean EGLAPIENTRY
eglStreamPostD3DTextureNV12ANGLE(EGLDisplay dpy,
                                 EGLStreamKHR stream,
                                 void* texture,
                                 const EGLAttrib* attrib_list);
#endif
#endif

#ifdef __cplusplus
}
#endif

#define  // UI_GL_EGL_EGLEXTCHROMIUM_H_
