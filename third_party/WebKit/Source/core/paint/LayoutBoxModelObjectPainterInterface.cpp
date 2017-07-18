// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "LayoutBoxModelObjectPainterInterface.h"

#include "core/layout/LayoutBoxModelObject.h"

namespace blink {

LayoutRectOutsets LayoutBoxModelObjectPainterInterface::BorderOutsets(
    const BoxPainterBase::FillLayerInfo& info) const {
  return LayoutRectOutsets(
      box_model_.BorderTop(),
      info.include_right_edge ? box_model_.BorderRight() : LayoutUnit(),
      box_model_.BorderBottom(),
      info.include_left_edge ? box_model_.BorderLeft() : LayoutUnit());
}

LayoutRectOutsets LayoutBoxModelObjectPainterInterface::PaddingOutsets(
    const BoxPainterBase::FillLayerInfo& info) const {
  return LayoutRectOutsets(
      box_model_.PaddingTop(),
      info.include_right_edge ? box_model_.PaddingRight() : LayoutUnit(),
      box_model_.PaddingBottom(),
      info.include_left_edge ? box_model_.PaddingLeft() : LayoutUnit());
}

const Document& LayoutBoxModelObjectPainterInterface::GetDocument() const {
  return box_model_.GetDocument();
}

const ComputedStyle& LayoutBoxModelObjectPainterInterface::Style() const {
  return box_model_.StyleRef();
}

LayoutRectOutsets LayoutBoxModelObjectPainterInterface::BorderPaddingInsets()
    const {
  return box_model_.BorderPaddingInsets();
}

bool LayoutBoxModelObjectPainterInterface::HasOverflowClip() const {
  return box_model_.HasOverflowClip();
}

Node* LayoutBoxModelObjectPainterInterface::GeneratingNode() const {
  Node* node = nullptr;
  const LayoutObject* layout_object = &box_model_;
  for (; layout_object && !node; layout_object = layout_object->Parent()) {
    node = layout_object->GeneratingNode();
  }
  return node;
}

LayoutBoxModelObjectPainterInterface::operator const DisplayItemClient&()
    const {
  return box_model_;
}

}  // namespace blink
