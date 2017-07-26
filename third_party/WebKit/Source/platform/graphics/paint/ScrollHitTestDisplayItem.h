// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ScrollHitTestDisplayItem_h
#define ScrollHitTestDisplayItem_h

#include "base/memory/ref_counted.h"
#include "platform/PlatformExport.h"
#include "platform/RuntimeEnabledFeatures.h"
#include "platform/graphics/paint/DisplayItem.h"

namespace blink {

class GraphicsContext;

// Placeholder display item for creating a special cc::Layer marked as being
// scrollable in PaintArtifactCompositor. A display item is needed because
// scroll hit testing must be in paint order.
class PLATFORM_EXPORT ScrollHitTestDisplayItem final : public DisplayItem {
 public:
  ScrollHitTestDisplayItem(const DisplayItemClient&, Type);
  ~ScrollHitTestDisplayItem();

  // DisplayItem
  void Replay(GraphicsContext&) const override;
  void AppendToWebDisplayItemList(const LayoutSize&,
                                  WebDisplayItemList*) const override;
  bool DrawsContent() const override;
  bool Equals(const DisplayItem&) const override;
#ifndef NDEBUG
  void DumpPropertiesAsDebugString(StringBuilder&) const override;
#endif
};

// Use this where you would use a recorder class.
PLATFORM_EXPORT void RecordScrollHitTest(GraphicsContext&,
                                         const DisplayItemClient&,
                                         DisplayItem::Type);

}  // namespace blink

#endif  // ScrollHitTestDisplayItem_h
