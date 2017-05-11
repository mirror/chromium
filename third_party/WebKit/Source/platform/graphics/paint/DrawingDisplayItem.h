// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DrawingDisplayItem_h
#define DrawingDisplayItem_h

#include "base/compiler_specific.h"
#include "platform/PlatformExport.h"
#include "platform/RuntimeEnabledFeatures.h"
#include "platform/geometry/FloatRect.h"
#include "platform/graphics/paint/DisplayItem.h"
#include "platform/graphics/paint/PaintRecord.h"
#include "third_party/skia/include/core/SkRefCnt.h"

namespace blink {

class PLATFORM_EXPORT DrawingDisplayItem final : public DisplayItem {
 public:
  DISABLE_CFI_PERF
  DrawingDisplayItem(const DisplayItemClient& client,
                     Type type,
                     sk_sp<const PaintRecord> record,
                     const FloatRect& cull_rect,
                     bool known_to_be_opaque = false)
      : DisplayItem(client, type, sizeof(*this)),
        record_(record && record->approximateOpCount() ? std::move(record)
                                                       : nullptr),
        cull_rect_(cull_rect),
        known_to_be_opaque_(known_to_be_opaque) {
    DCHECK(IsDrawingType(type));
  }

  void Replay(GraphicsContext&) const override;
  void AppendToWebDisplayItemList(const IntRect&,
                                  WebDisplayItemList*) const override;
  bool DrawsContent() const override;

  const sk_sp<const PaintRecord>& GetPaintRecord() const { return record_; }
  FloatRect CullRect() const { return cull_rect_; }

  bool KnownToBeOpaque() const {
    DCHECK(RuntimeEnabledFeatures::slimmingPaintV2Enabled());
    return known_to_be_opaque_;
  }

  int NumberOfSlowPaths() const override;

 private:
#ifndef NDEBUG
  void DumpPropertiesAsDebugString(WTF::StringBuilder&) const override;
#endif
  bool Equals(const DisplayItem& other) const final;

  sk_sp<const PaintRecord> record_;
  FloatRect cull_rect_;

  // True if there are no transparent areas. Only used for SlimmingPaintV2.
  const bool known_to_be_opaque_;
};

}  // namespace blink

#endif  // DrawingDisplayItem_h
