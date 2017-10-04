// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "modules/accessibility/testing/AccessibilityTest.h"

#include "core/dom/Document.h"
#include "core/frame/LocalFrameClient.h"
#include "core/frame/Settings.h"
#include "core/layout/LayoutView.h"
#include "modules/accessibility/AXObject.h"
#include "modules/accessibility/AXObjectCacheImpl.h"

namespace blink {

void AccessibilityTest::SetUp() {
  RenderingTest::SetUp();
  DCHECK(GetDocument().GetSettings());
  GetDocument().GetSettings()->SetAccessibilityEnabled(true);
}

void AccessibilityTest::TearDown() {
  GetDocument().ClearAXObjectCache();
  DCHECK(GetDocument().GetSettings());
  GetDocument().GetSettings()->SetAccessibilityEnabled(false);
  RenderingTest::TearDown();
}

AXObjectCacheImpl& AccessibilityTest::GetAXObjectCache() const {
  auto ax_object_cache = ToAXObjectCacheImpl(GetDocument().AxObjectCache());
  DCHECK(ax_object_cache);
  return *ax_object_cache;
}

AXObject* AccessibilityTest::GetAXRootObject() const {
  return GetAXObjectCache().Get(&GetLayoutView());
}

AXObject* AccessibilityTest::GetAXFocusedObject() const {
  return GetAXObjectCache().FocusedObject();
}

AXObject* AccessibilityTest::GetAXObjectByElementId(const char* id) const {
  const auto* element = GetElementById(id);
  return element ? GetAXObjectCache().Get(element) : nullptr;
}

std::ostream& AccessibilityTest::PrintAXTree(std::ostream& stream) const {
  return (stream << PrintAXTree(stream, GetAXRootObject(), 0));
}

std::ostream& AccessibilityTest::PrintAXTree(std::ostream& stream,
                                             const AXObject* root,
                                             size_t level) const {
  if (!root)
    return stream;
  stream << std::string(level, '+');
  stream << *root << std::endl;
  for (const auto child : const_cast<AXObject*>(root)->Children()) {
    DCHECK(child);
    stream << (PrintAXTree(stream, child.Get(), level + 1) << std::endl);
  }
  return stream;
}

}  // namespace blink
