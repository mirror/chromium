// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LayoutEmbeddedContentViewItem_h
#define LayoutEmbeddedContentViewItem_h

#include "core/layout/LayoutEmbeddedContentView.h"
#include "core/layout/api/LayoutBoxItem.h"

namespace blink {

class LayoutEmbeddedContentViewItem : public LayoutBoxItem {
 public:
  explicit LayoutEmbeddedContentViewItem(
      LayoutEmbeddedContentView* layout_embedded_content_view)
      : LayoutBoxItem(layout_embedded_content_view) {}

  explicit LayoutEmbeddedContentViewItem(const LayoutItem& item)
      : LayoutBoxItem(item) {
    SECURITY_DCHECK(!item || item.IsLayoutEmbeddedContentView());
  }

  explicit LayoutEmbeddedContentViewItem(std::nullptr_t)
      : LayoutBoxItem(nullptr) {}

  LayoutEmbeddedContentViewItem() {}

  void UpdateOnEmbeddedContentViewChange() {
    ToPart()->UpdateOnEmbeddedContentViewChange();
  }

 private:
  LayoutEmbeddedContentView* ToPart() {
    return ToLayoutEmbeddedContentView(GetLayoutObject());
  }

  const LayoutEmbeddedContentView* ToEmbeddedContentView() const {
    return ToLayoutEmbeddedContentView(GetLayoutObject());
  }
};

}  // namespace blink

#endif  // LayoutEmbeddedContentViewItem_h
