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
const CSSValue* CSSPropertyAPITextEmphasisPosition::ParseSingleValue(
    CSSPropertyID,
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  CSSIdentifierValue* first = CSSPropertyParserHelpers::ConsumeIdent<
      CSSValueOver, CSSValueUnder, CSSValueRight, CSSValueLeft>(range);
  if (!first) {
    return nullptr;
  }
  CSSIdentifierValue* second = CSSPropertyParserHelpers::ConsumeIdent<
      CSSValueOver, CSSValueUnder, CSSValueRight, CSSValueLeft>(range);

  CSSIdentifierValue* value = nullptr;
  if (!second) {
    switch (first->GetValueID()) {
      case CSSValueOver:
        value = CSSIdentifierValue::Create(CSSValueOverRight);
        break;
      case CSSValueUnder:
        value = CSSIdentifierValue::Create(CSSValueUnderLeft);
        break;
      case CSSValueRight:
        value = CSSIdentifierValue::Create(CSSValueOverRight);
        break;
      case CSSValueLeft:
        value = CSSIdentifierValue::Create(CSSValueUnderLeft);
        break;
      default:
        value = CSSIdentifierValue::Create(CSSValueOverRight);
        break;
    }
    return value;
  }

  if (first->GetValueID() == CSSValueOver) {
    switch (second->GetValueID()) {
      case CSSValueRight:
        value = CSSIdentifierValue::Create(CSSValueOverRight);
        break;
      case CSSValueLeft:
        value = CSSIdentifierValue::Create(CSSValueOverLeft);
        break;
      default:
        value = CSSIdentifierValue::Create(CSSValueOverRight);
        break;
    }
  } else if (first->GetValueID() == CSSValueUnder) {
    switch (second->GetValueID()) {
      case CSSValueRight:
        value = CSSIdentifierValue::Create(CSSValueUnderRight);
        break;
      case CSSValueLeft:
        value = CSSIdentifierValue::Create(CSSValueUnderLeft);
        break;
      default:
        value = CSSIdentifierValue::Create(CSSValueOverRight);
        break;
    }
  } else if (first->GetValueID() == CSSValueRight) {
    switch (second->GetValueID()) {
      case CSSValueOver:
        value = CSSIdentifierValue::Create(CSSValueOverRight);
        break;
      case CSSValueUnder:
        value = CSSIdentifierValue::Create(CSSValueUnderRight);
        break;
      default:
        value = CSSIdentifierValue::Create(CSSValueOverRight);
        break;
    }
  } else {
    switch (second->GetValueID()) {
      case CSSValueOver:
        value = CSSIdentifierValue::Create(CSSValueOverLeft);
        break;
      case CSSValueUnder:
        value = CSSIdentifierValue::Create(CSSValueUnderLeft);
        break;
      default:
        value = CSSIdentifierValue::Create(CSSValueOverRight);
        break;
    }
  }
  return value;
}

}  // namespace blink
