// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/StyleSheetContents.h"
#include "core/css/parser/CSSParser.h"

#include <string>

#include "testing/libfuzzer/fuzzers/css.pb.h"
#include "testing/libfuzzer/fuzzers/css_proto_converter.h"
#include "third_party/libprotobuf-mutator/src/src/libfuzzer/libfuzzer_macro.h"

#include "platform/testing/BlinkFuzzerTestSupport.h"
#include "platform/wtf/text/WTFString.h"

protobuf_mutator::protobuf::LogSilencer log_silencer;

namespace css_parser_proto_fuzzer {

DEFINE_BINARY_PROTO_FUZZER(
    const css_proto_converter::StyleSheet& style_sheet_message) {
  static css_proto_converter::Converter* converter;
  static bool initialized = false;

  if (!initialized) {
    static blink::BlinkFuzzerTestSupport test_support =
        blink::BlinkFuzzerTestSupport();
    initialized = true;
    converter = new css_proto_converter::Converter();
  }

  blink::CSSParserContext* context =
      blink::CSSParserContext::Create(blink::kHTMLStandardMode);
  blink::StyleSheetContents* style_sheet =
      blink::StyleSheetContents::Create(context);
  WTF::String style_sheet_string(
      converter->Convert(style_sheet_message).c_str());
  blink::CSSParser::ParseSheet(context, style_sheet, style_sheet_string, false);
}
};  // namespace css_parser_proto_fuzzer
