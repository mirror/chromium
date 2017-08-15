// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LegacyLayoutTreeWalking_h
#define LegacyLayoutTreeWalking_h

namespace blink {

class LayoutBlockFlow;
class LayoutObject;

// Return the layout object that should be the first child NGLayoutInputNode of
// |parent|. Normally this will just be the first layout object child, but there
// are certain layout objects that should be skipped for NG. |parent| will be
// updated to become the parent of the child actually returned.
LayoutObject* GetLayoutObjectForFirstChildNode(LayoutBlockFlow*& parent);

// Return the layout object that should be the parent NGLayoutInputNode of
// |object|. Normally this will just be the parent layout object, but there
// are certain layout objects that should be skipped for NG.
LayoutObject* GetLayoutObjectForParentNode(LayoutObject* object);

}  // namespace blink

#endif  // LegacyLayoutTreeWalking_h
