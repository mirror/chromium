// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/longhands/Transform.h"

#include "core/css/parser/CSSParserContext.h"
#include "core/css/parser/CSSParserLocalContext.h"
#include "core/css/parser/CSSParserTokenRange.h"
#include "core/css/properties/CSSPropertyTransformUtils.h"
#include "core/layout/LayoutObject.h"

namespace blink {

const CSSValue* CSSLonghand::Transform::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext& local_context) const {
  return CSSPropertyTransformUtils::ConsumeTransformList(range, context,
                                                         local_context);
}

bool CSSLonghand::Transform::IsLayoutDependent(
    const ComputedStyle* style,
    LayoutObject* layout_object) const {
  return layout_object && layout_object->IsBox();
}

}  // namespace blink
