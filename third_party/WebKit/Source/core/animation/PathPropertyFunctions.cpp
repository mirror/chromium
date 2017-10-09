// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/animation/PathPropertyFunctions.h"

#include "core/style/ComputedStyle.h"

namespace blink {

StylePath* PathPropertyFunctions::GetPath(CSSPropertyID property,
                                          const ComputedStyle& style) {
  switch (property) {
    case CSSPropertyD:
      return style.SvgStyle().D();
    case CSSPropertyOffsetPath: {
      BasicShape* offset_path = style.OffsetPath();
      if (!offset_path || offset_path->GetType() != BasicShape::kStylePathType)
        return nullptr;
      return ToStylePath(style.OffsetPath());
    }
    default:
      NOTREACHED();
      return nullptr;
  }
}

void PathPropertyFunctions::SetPath(CSSPropertyID property,
                                    ComputedStyle& style,
                                    RefPtr<blink::StylePath> path) {
  switch (property) {
    case CSSPropertyD:
      style.SetD(std::move(path));
      return;
    case CSSPropertyOffsetPath:
      style.SetOffsetPath(std::move(path));
      return;
    default:
      NOTREACHED();
      return;
  }
}

}  // namespace blink
