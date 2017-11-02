// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebGLVideoFrameInfo_h
#define WebGLVideoFrameInfo_h

#include "platform/bindings/ScriptWrappable.h"

namespace blink {

class WebGLVideoFrameInfo final : public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
public:
    static WebGLVideoFrameInfo* Create(
        double frameTime, GLint textureWidth, GLint textureHeight)
    {
        return new WebGLVideoFrameInfo(frameTime, textureWidth, textureHeight);
    }

    double frameTime() const { return m_frameTime; }
    GLint textureWidth() const { return m_textureWidth; }
    GLint textureHeight() const { return m_textureHeight; }

    virtual void Trace(blink::Visitor*);

private:
    WebGLVideoFrameInfo(
        double frameTime, GLint textureWidth, GLint textureHeight)
        : m_frameTime(frameTime)
        , m_textureWidth(textureWidth)
        , m_textureHeight(textureHeight) {}
    double m_frameTime;
    GLint m_textureWidth;
    GLint m_textureHeight;
};

} // namespace blink

#endif // WebGLVideoFrameInfo_h
