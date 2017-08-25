// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/CSSPropertyAPITextEmphasisPosition.h"

#include "core/css/CSSIdentifierValue.h"
#include "core/css/CSSValueList.h"
#include "core/css/parser/CSSPropertyParserHelpers.h"

class CSSParserLocalContext;
namespace blink {

// [ over | under ] && [ right | left ]?
// If [ right | left ] is omitted, it defaults to right.
const CSSValue* CSSPropertyAPITextEmphasisPosition::ParseSingleValue(
    CSSPropertyID,
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  CSSValueList* list = CSSValueList::CreateSpaceSeparated();
  CSSIdentifierValue* first = CSSPropertyParserHelpers::ConsumeIdent<
      CSSValueOver, CSSValueUnder, CSSValueRight, CSSValueLeft>(range);
  if (!first) {
    return nullptr;
  }
  CSSIdentifierValue* second = CSSPropertyParserHelpers::ConsumeIdent<
      CSSValueOver, CSSValueUnder, CSSValueRight, CSSValueLeft>(range);

  if (!second) {
    switch (first->GetValueID()) {
      case CSSValueOver:
      case CSSValueUnder:
        list->Append(*first);
        list->Append(*CSSIdentifierValue::Create(CSSValueRight));
        break;
      default:
        list->Append(*CSSIdentifierValue::Create(CSSValueOver));
        list->Append(*CSSIdentifierValue::Create(CSSValueRight));
        break;
    }
    return list;
  }

  if (first->GetValueID() == CSSValueOver ||
      first->GetValueID() == CSSValueUnder) {
    switch (second->GetValueID()) {
      case CSSValueRight:
      case CSSValueLeft:
        list->Append(*first);
        list->Append(*second);
        break;
      default:
        list->Append(*CSSIdentifierValue::Create(CSSValueOver));
        list->Append(*CSSIdentifierValue::Create(CSSValueRight));
        break;
    }
  } else {
    switch (second->GetValueID()) {
      case CSSValueOver:
      case CSSValueUnder:
        list->Append(*second);
        list->Append(*first);
        break;
      default:
        list->Append(*CSSIdentifierValue::Create(CSSValueOver));
        list->Append(*CSSIdentifierValue::Create(CSSValueRight));
        break;
    }
  }
  return list;
}

}  // namespace blink
