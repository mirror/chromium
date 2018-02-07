// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LayoutCustom_h
#define LayoutCustom_h

#include "core/layout/LayoutBlockFlow.h"

namespace blink {

// The LayoutObject for elements which have "display: layout(foo);" specified.
// https://drafts.css-houdini.org/css-layout-api/
//
// This class inherits from LayoutBlockFlow so that when a web developer's
// intrinsicSizes/layout callback fails, it will fallback onto the default
// block-flow layout algorithm.
class LayoutCustom final : public LayoutBlockFlow {
 public:
  explicit LayoutCustom(Element*);
  ~LayoutCustom() override;

  const char* GetName() const override { return "LayoutCustom"; }

  bool CreatesNewFormattingContext() const override { return true; }

  void StyleDidChange(StyleDifference, const ComputedStyle* old_style) override;

 private:
  bool IsOfType(LayoutObjectType type) const override {
    return type == kLayoutObjectLayoutCustom || LayoutBlock::IsOfType(type);
  }
};

DEFINE_LAYOUT_OBJECT_TYPE_CASTS(LayoutCustom, IsLayoutCustom());

}  // namespace blink

#endif  // LayoutCustom_h
