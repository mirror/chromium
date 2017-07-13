// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ScrollLayerDisplayItem_h
#define ScrollLayerDisplayItem_h

#include "base/memory/ref_counted.h"
#include "platform/PlatformExport.h"
#include "platform/RuntimeEnabledFeatures.h"
#include "platform/graphics/paint/DisplayItem.h"

namespace blink {

class GraphicsContext;

// TODO(pdr): Comment this.
class PLATFORM_EXPORT ScrollLayerDisplayItem final : public DisplayItem {
 public:
  ScrollLayerDisplayItem(const DisplayItemClient&, Type);
  ~ScrollLayerDisplayItem();

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
PLATFORM_EXPORT void RecordScrollLayer(GraphicsContext&,
                                       const DisplayItemClient&,
                                       DisplayItem::Type);

}  // namespace blink

#endif  // ScrollLayerDisplayItem_h
