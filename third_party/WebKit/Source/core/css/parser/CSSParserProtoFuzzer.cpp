// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/parser/CSSParserProtoConverter.h"

#include <unordered_map>

#include "third_party/libprotobuf-mutator/src/src/libfuzzer/libfuzzer_macro.h"

#include "core/css/StyleSheetContents.h"
#include "core/css/parser/CSSParser.h"
#include "platform/testing/BlinkFuzzerTestSupport.h"
#include "platform/wtf/text/WTFString.h"
#include "third_party/WebKit/Source/core/css/parser/CSS.pb.h"

protobuf_mutator::protobuf::LogSilencer log_silencer;

namespace css_parser_proto_fuzzer {

DEFINE_BINARY_PROTO_FUZZER(const Input& input) {
  static Converter* converter;
  static bool initialized = false;
  if (!initialized) {
    static blink::BlinkFuzzerTestSupport test_support =
        blink::BlinkFuzzerTestSupport();
    initialized = true;
    converter = new Converter();
  }
  static std::unordered_map<Input::CSSParserMode, blink::CSSParserMode>
      parser_mode_map = {
          {Input::kHTMLStandardMode, blink::kHTMLStandardMode},
          {Input::kHTMLQuirksMode, blink::kHTMLQuirksMode},
          {Input::kSVGAttributeMode, blink::kSVGAttributeMode},
          {Input::kCSSViewportRuleMode, blink::kCSSViewportRuleMode},
          {Input::kCSSFontFaceRuleMode, blink::kCSSFontFaceRuleMode},
          {Input::kUASheetMode, blink::kUASheetMode}};

  blink::CSSParserMode mode = parser_mode_map[input.css_parser_mode()];

  blink::CSSParserContext::SelectorProfile selector_profile;
  if (input.is_dynamic_profile())
    selector_profile = blink::CSSParserContext::kDynamicProfile;
  else
    selector_profile = blink::CSSParserContext::kStaticProfile;
  blink::CSSParserContext* context =
      blink::CSSParserContext::Create(mode, selector_profile);

  blink::StyleSheetContents* style_sheet =
      blink::StyleSheetContents::Create(context);
#include <string>
  std::string s = converter->Convert(input.style_sheet());
  WTF::String style_sheet_string(s.c_str());
#include <iostream>
  // std::cout << s << std::endl;
  blink::CSSParser::ParseSheet(context, style_sheet, style_sheet_string,
                               input.defer_property_parsing());
}
};  // namespace css_parser_proto_fuzzer
