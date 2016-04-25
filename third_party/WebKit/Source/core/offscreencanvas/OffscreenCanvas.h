// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OffscreenCanvas_h
#define OffscreenCanvas_h

#include "bindings/core/v8/ScriptPromise.h"
#include "bindings/core/v8/ScriptState.h"
#include "bindings/core/v8/ScriptWrappable.h"
#include "core/html/HTMLCanvasElement.h"
#include "platform/geometry/IntSize.h"
#include "platform/heap/Handle.h"

namespace blink {

class CanvasContextCreationAttributes;
class ImageBitmap;

class CORE_EXPORT OffscreenCanvas final : public GarbageCollectedFinalized<OffscreenCanvas>, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
public:
    static OffscreenCanvas* create(unsigned width, unsigned height);
    ~OffscreenCanvas();

    // IDL attributes
    unsigned width() const { return m_size.width(); }
    unsigned height() const { return m_size.height(); }
    void setWidth(unsigned);
    void setHeight(unsigned);

    // API Methods
    ImageBitmap* transferToImageBitmap(ExceptionState&);

    IntSize size() const { return m_size; }
    void setAssociatedCanvas(HTMLCanvasElement* canvas) { m_canvas = canvas; }
    HTMLCanvasElement* getAssociatedCanvas() const { return m_canvas; }
    CanvasRenderingContext* getCanvasRenderingContext(const String&, const CanvasContextCreationAttributes&);

    static void registerRenderingContextFactory(PassOwnPtr<CanvasRenderingContextFactory>);

    DECLARE_VIRTUAL_TRACE();

private:
    OffscreenCanvas(const IntSize&);

    using ContextFactoryVector = Vector<OwnPtr<CanvasRenderingContextFactory>>;
    static ContextFactoryVector& renderingContextFactories();
    static CanvasRenderingContextFactory* getRenderingContextFactory(int);

    Member<CanvasRenderingContext> m_context;
    WeakMember<HTMLCanvasElement> m_canvas;
    IntSize m_size;
};

} // namespace blink

#endif // OffscreenCanvas_h
