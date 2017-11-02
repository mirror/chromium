// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebGLVideoTexture_h
#define WebGLVideoTexture_h

#include "modules/webgl/WebGLExtension.h"
#include "modules/webgl/WebGLVideoFrameInfo.h"

namespace blink {

class ExceptionState;
class HTMLVideoElement;
class WebGLVideoFrameInfo;

class WebGLVideoTexture final : public WebGLExtension {
    DEFINE_WRAPPERTYPEINFO();

public:
    static WebGLVideoTexture* Create(WebGLRenderingContextBase*);
    static bool Supported(WebGLRenderingContextBase*);
    static const char* ExtensionName();

    WebGLExtensionName GetName() const override;

    WebGLVideoFrameInfo* VideoElementTargetVideoTexture(
        GLenum target, HTMLVideoElement*, ExceptionState&);

private:
    explicit WebGLVideoTexture(WebGLRenderingContextBase*);
};

} // namespace blink

#endif // WebGLVideoTexture_h
