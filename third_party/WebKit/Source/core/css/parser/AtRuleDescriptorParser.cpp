// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/parser/AtRuleDescriptorParser.h"

#include "core/css/CSSFontFaceSrcValue.h"
#include "core/css/CSSUnicodeRangeValue.h"
#include "core/css/CSSValue.h"
#include "core/css/parser/CSSParserContext.h"
#include "core/css/parser/CSSParserMode.h"
#include "core/css/parser/CSSParserTokenRange.h"
#include "core/css/parser/CSSTokenizer.h"
#include "core/css/properties/CSSPropertyFontUtils.h"

namespace blink {

using namespace CSSPropertyParserHelpers;

namespace {

CSSParserTokenRange ToCSSParserTokenRange(const String& value) {
  CSSTokenizer tokenizer(value);
  return CSSParserTokenRange(tokenizer.TokenizeToEOF());
}

CSSValue* ConsumeFontVariantList(CSSParserTokenRange& range) {
  CSSValueList* values = CSSValueList::CreateCommaSeparated();
  do {
    if (range.Peek().Id() == CSSValueAll) {
      // FIXME: CSSPropertyParser::ParseFontVariant() implements
      // the old css3 draft:
      // http://www.w3.org/TR/2002/WD-css3-webfonts-20020802/#font-variant
      // 'all' is only allowed in @font-face and with no other values.
      if (values->length())
        return nullptr;
      return ConsumeIdent(range);
    }
    CSSIdentifierValue* font_variant =
        CSSPropertyFontUtils::ConsumeFontVariantCSS21(range);
    if (font_variant)
      values->Append(*font_variant);
  } while (ConsumeCommaIncludingWhitespace(range));

  if (values->length())
    return values;

  return nullptr;
}

CSSIdentifierValue* ConsumeFontDisplay(CSSParserTokenRange& range) {
  return ConsumeIdent<CSSValueAuto, CSSValueBlock, CSSValueSwap,
                      CSSValueFallback, CSSValueOptional>(range);
}

CSSValueList* ConsumeFontFaceUnicodeRange(CSSParserTokenRange& range) {
  CSSValueList* values = CSSValueList::CreateCommaSeparated();

  do {
    const CSSParserToken& token = range.ConsumeIncludingWhitespace();
    if (token.GetType() != kUnicodeRangeToken)
      return nullptr;

    UChar32 start = token.UnicodeRangeStart();
    UChar32 end = token.UnicodeRangeEnd();
    if (start > end)
      return nullptr;
    values->Append(*CSSUnicodeRangeValue::Create(start, end));
  } while (ConsumeCommaIncludingWhitespace(range));

  return values;
}

CSSValue* ConsumeFontFaceSrcURI(CSSParserTokenRange& range,
                                const CSSParserContext& context) {
  String url = ConsumeUrlAsStringView(range).ToString();
  if (url.IsNull())
    return nullptr;
  CSSFontFaceSrcValue* uri_value(CSSFontFaceSrcValue::Create(
      url, context.CompleteURL(url), context.GetReferrer(),
      context.ShouldCheckContentSecurityPolicy()));

  if (range.Peek().FunctionId() != CSSValueFormat)
    return uri_value;

  // FIXME: https://drafts.csswg.org/css-fonts says that format() contains a
  // comma-separated list of strings, but CSSFontFaceSrcValue stores only one
  // format. Allowing one format for now.
  CSSParserTokenRange args = ConsumeFunction(range);
  const CSSParserToken& arg = args.ConsumeIncludingWhitespace();
  if ((arg.GetType() != kStringToken) || !args.AtEnd())
    return nullptr;
  uri_value->SetFormat(arg.Value().ToString());
  return uri_value;
}

CSSValue* ConsumeFontFaceSrcLocal(CSSParserTokenRange& range,
                                  const CSSParserContext& context) {
  CSSParserTokenRange args = ConsumeFunction(range);
  ContentSecurityPolicyDisposition should_check_content_security_policy =
      context.ShouldCheckContentSecurityPolicy();
  if (args.Peek().GetType() == kStringToken) {
    const CSSParserToken& arg = args.ConsumeIncludingWhitespace();
    if (!args.AtEnd())
      return nullptr;
    return CSSFontFaceSrcValue::CreateLocal(
        arg.Value().ToString(), should_check_content_security_policy);
  }
  if (args.Peek().GetType() == kIdentToken) {
    String family_name = CSSPropertyFontUtils::ConcatenateFamilyName(args);
    if (!args.AtEnd())
      return nullptr;
    return CSSFontFaceSrcValue::CreateLocal(
        family_name, should_check_content_security_policy);
  }
  return nullptr;
}

CSSValueList* ConsumeFontFaceSrc(CSSParserTokenRange& range,
                                 const CSSParserContext& context) {
  CSSValueList* values = CSSValueList::CreateCommaSeparated();

  do {
    const CSSParserToken& token = range.Peek();
    CSSValue* parsed_value = nullptr;
    if (token.FunctionId() == CSSValueLocal)
      parsed_value = ConsumeFontFaceSrcLocal(range, context);
    else
      parsed_value = ConsumeFontFaceSrcURI(range, context);
    if (!parsed_value)
      return nullptr;
    values->Append(*parsed_value);
  } while (ConsumeCommaIncludingWhitespace(range));
  return values;
}

}  // namespace

CSSValue* AtRuleDescriptorParser::ParseFontFaceDescriptor(
    AtRuleDescriptorID id,
    const String& value,
    const CSSParserContext& context) {
  CSSParserTokenRange range = ToCSSParserTokenRange(value);
  CSSValue* parsed_value = nullptr;
  switch (id) {
    case AtRuleDescriptorID::FontFamily:
      if (CSSPropertyFontUtils::ConsumeGenericFamily(range))
        return nullptr;
      parsed_value = CSSPropertyFontUtils::ConsumeFamilyName(range);
      break;
    case AtRuleDescriptorID::Src:  // This is a list of urls or local
                                   // references.
      parsed_value = ConsumeFontFaceSrc(range, context);
      break;
    case AtRuleDescriptorID::UnicodeRange:
      parsed_value = ConsumeFontFaceUnicodeRange(range);
      break;
    case AtRuleDescriptorID::FontDisplay:
      parsed_value = ConsumeFontDisplay(range);
      break;
    case AtRuleDescriptorID::FontStretch:
      parsed_value =
          CSSPropertyFontUtils::ConsumeFontStretch(range, kCSSFontFaceRuleMode);
      break;
    case AtRuleDescriptorID::FontStyle:
      parsed_value =
          CSSPropertyFontUtils::ConsumeFontStyle(range, kCSSFontFaceRuleMode);
      break;
    case AtRuleDescriptorID::FontVariant:
      parsed_value = ConsumeFontVariantList(range);
      break;
    case AtRuleDescriptorID::FontWeight:
      parsed_value =
          CSSPropertyFontUtils::ConsumeFontWeight(range, kCSSFontFaceRuleMode);
      break;
    case AtRuleDescriptorID::FontFeatureSettings:
      parsed_value = CSSPropertyFontUtils::ConsumeFontFeatureSettings(range);
      break;
    default:
      break;
  }

  if (!parsed_value || !range.AtEnd())
    return nullptr;

  return parsed_value;
}

}  // namespace blink
