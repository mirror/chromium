// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/page/scrolling/RootScrollerUtil.h"

#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/frame/FrameView.h"
#include "core/layout/LayoutBox.h"
#include "core/layout/LayoutBoxModelObject.h"
#include "core/layout/LayoutView.h"
#include "core/paint/PaintLayerScrollableArea.h"

namespace blink {

namespace RootScrollerUtil {

ScrollableArea* scrollableAreaForRootScroller(const Element& element) {
  if (&element == element.document().documentElement()) {
    if (!element.document().view())
      return nullptr;

    // For a FrameView, we use the layoutViewport rather than the
    // getScrollableArea() since that could be the RootFrameViewport. The
    // rootScroller's ScrollableArea will be swapped in as the layout viewport
    // in RootFrameViewport so we need to ensure we get the layout viewport.
    return element.document().view()->layoutViewportScrollableArea();
  }

  if (!element.layoutObject() || !element.layoutObject()->isBox())
    return nullptr;

  return static_cast<PaintInvalidationCapableScrollableArea*>(
      toLayoutBoxModelObject(element.layoutObject())->getScrollableArea());
}

PaintLayer* paintLayerForRootScroller(const Element* element) {
  if (!element || !element->layoutObject())
    return nullptr;

  DCHECK(element->layoutObject()->isBox());
  LayoutBox* box = toLayoutBox(element->layoutObject());

  // If the root scroller is the <html> element we do a bit of a fake out
  // because while <html> has a PaintLayer, scrolling for it is handled by the
  // #document's PaintLayer (i.e. the PaintLayerCompositor's root layer). The
  // reason the root scroller is the <html> layer and not #document is because
  // the latter is a Node but not an Element.
  if (element->isSameNode(element->document().documentElement())) {
    if (!box->view())
      return nullptr;
    return box->view()->layer();
  }

  return box->layer();
}

}  // namespace RootScrollerUtil

}  // namespace blink
