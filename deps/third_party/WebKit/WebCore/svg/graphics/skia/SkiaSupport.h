// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Helper methods for Skia SVG rendering, inspired by CgSupport.*

// TODO: REMOVE this file, it is no longer meaningful.

#ifndef SkiaSupport_h
#define SkiaSupport_h

#if ENABLE(SVG)

namespace WebCore {

class GraphicsContext;

// Returns a statically allocated 1x1 GraphicsContext intended for temporary
// operations. Please save() the state and restore() it when you're done with
// the context.
GraphicsContext* scratchContext();

}


#endif // ENABLE(SVG)
#endif // #ifndef SkiaSupport_h

