// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BoxModelPainterInterface_h
#define BoxModelPainterInterface_h

namespace blink {
class LayoutBox;
class BoxPainterInterface;
class Node;
class LayoutRectOutsets;
class DisplayItemClient;
class LayoutBoxModelObject;

// Abstract interface between the layout and paint code for box model object
// painting.
class BoxModelPainterInterface {
 public:
  virtual bool IsLayoutNG() const = 0;
  virtual bool IsBox() const = 0;
  virtual LayoutRectOutsets BorderOutsets(
      const BoxPainterBase::FillLayerInfo&) const = 0;
  virtual LayoutRectOutsets PaddingOutsets(
      const BoxPainterBase::FillLayerInfo&) const = 0;
  virtual const Document& GetDocument() const = 0;
  virtual const ComputedStyle& Style() const = 0;
  virtual LayoutRectOutsets BorderPaddingInsets() const = 0;
  virtual bool HasOverflowClip() const = 0;
  virtual Node* GeneratingNode() const = 0;
  virtual const BoxPainterInterface* ToBoxPainterInterface() const {
    return nullptr;
  }
  virtual const LayoutBoxModelObject* GetLayoutBoxModelObject() const {
    return nullptr;
  }
  virtual operator const DisplayItemClient&() const = 0;
};

} // namesapce blink

#endif // BoxModelPainterInterface_h
