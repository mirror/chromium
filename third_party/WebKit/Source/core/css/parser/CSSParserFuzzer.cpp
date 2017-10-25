// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/StyleSheetContents.h"
#include "core/css/parser/CSSParser.h"
#include "platform/testing/BlinkFuzzerTestSupport.h"
#include "platform/wtf/text/WTFString.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  static blink::BlinkFuzzerTestSupport test_support =
      blink::BlinkFuzzerTestSupport();

  const static blink::CSSParserMode kParserModeMap[] = {
      blink::kHTMLStandardMode,    blink::kHTMLQuirksMode,
      blink::kSVGAttributeMode,    blink::kCSSViewportRuleMode,
      blink::kCSSFontFaceRuleMode, blink::kUASheetMode};

  if (size <= 5)
    return 0;

  blink::CSSParserMode mode = kParserModeMap[data[0] % 6];

  blink::CSSParserContext::SelectorProfile selector_profile;

  if (data[1] % 2)
    selector_profile = blink::CSSParserContext::kDynamicProfile;
  else
    selector_profile = blink::CSSParserContext::kStaticProfile;

  blink::CSSParserContext* context =
      blink::CSSParserContext::Create(mode, selector_profile);
  blink::StyleSheetContents* style_sheet =
      blink::StyleSheetContents::Create(context);
  bool defer_property_parsing = data[4] % 2;
  WTF::String style_sheet_string(data + 4, size - 4);
  blink::CSSParser::ParseSheet(context, style_sheet, style_sheet_string,
                               defer_property_parsing);
  return 0;
}
