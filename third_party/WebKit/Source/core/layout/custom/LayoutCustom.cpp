// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/custom/LayoutCustom.h"

#include "core/dom/Document.h"

namespace blink {

LayoutCustom::LayoutCustom(Element* element) : LayoutBlockFlow(element) {
  DCHECK(!ChildrenInline());
}

LayoutCustom::~LayoutCustom() = default;

LayoutCustom* LayoutCustom::CreateAnonymous(Document* document) {
  LayoutCustom* layout_custom = new LayoutCustom(nullptr);
  layout_custom->SetDocumentForAnonymous(document);
  return layout_custom;
}

}  // namespace blink
