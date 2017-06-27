// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PaintPropertyTreePrinter_h
#define PaintPropertyTreePrinter_h

#include "core/CoreExport.h"
#include "platform/wtf/text/WTFString.h"

#if DCHECK_IS_ON()

namespace blink {

class LocalFrameView;

}  // namespace blink

// Outside the blink namespace for ease of invocation from gdb.
CORE_EXPORT_N2547 void showAllPropertyTrees(const blink::LocalFrameView& rootFrame);
CORE_EXPORT_N2548 void showTransformPropertyTree(
    const blink::LocalFrameView& rootFrame);
CORE_EXPORT_N2549 void showClipPropertyTree(const blink::LocalFrameView& rootFrame);
CORE_EXPORT_N2550 void showEffectPropertyTree(const blink::LocalFrameView& rootFrame);
CORE_EXPORT_N2551 void showScrollPropertyTree(const blink::LocalFrameView& rootFrame);
CORE_EXPORT_N2552 String
transformPropertyTreeAsString(const blink::LocalFrameView& rootFrame);
CORE_EXPORT_N2553 String
clipPropertyTreeAsString(const blink::LocalFrameView& rootFrame);
CORE_EXPORT_N2554 String
effectPropertyTreeAsString(const blink::LocalFrameView& rootFrame);
CORE_EXPORT_N2555 String
scrollPropertyTreeAsString(const blink::LocalFrameView& rootFrame);

CORE_EXPORT_N2556 String paintPropertyTreeGraph(const blink::LocalFrameView&);

#endif  // if DCHECK_IS_ON()

#endif  // PaintPropertyTreePrinter_h
