// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/CSSPropertyAPITextEmphasisPosition.h"

#include "core/css/CSSIdentifierValue.h"
#include "core/css/CSSValueList.h"
#include "core/css/parser/CSSPropertyParserHelpers.h"

class CSSParserLocalContext;
namespace blink {

// [ over | under ] && [ right | left ]
const CSSValue* CSSPropertyAPITextEmphasisPosition::parseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) {
  CSSValueList* list = CSSValueList::CreateSpaceSeparated();
  CSSIdentifierValue* first =
      CSSPropertyParserHelpers::ConsumeIdent<CSSValueOver, CSSValueUnder>(
          range);
  if (!first) {
    return nullptr;
  }
  list->Append(*first);
  CSSIdentifierValue* second =
      CSSPropertyParserHelpers::ConsumeIdent<CSSValueLeft, CSSValueRight>(
          range);
  if (!second) {
    list->Append(first->GetValueID() == CSSValueOver
                     ? *CSSIdentifierValue::Create(CSSValueRight)
                     : *CSSIdentifierValue::Create(CSSValueLeft));
  } else {
    list->Append(*second);
  }

  return list;
}

}  // namespace blink
