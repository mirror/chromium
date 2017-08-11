// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CSSPropertyBackgroundUtils_h
#define CSSPropertyBackgroundUtils_h

#include "core/css/parser/CSSPropertyParserHelpers.h"
#include "platform/wtf/Allocator.h"

namespace blink {

class CSSParserContext;
class CSSParserTokenRange;
class CSSValue;

class CSSPropertyBackgroundUtils {
  STATIC_ONLY(CSSPropertyBackgroundUtils);

 public:
  // TODO(jiameng): move AddBackgroundValue to anonymous namespace once all
  // background-related properties have their APIs.
  static void AddBackgroundValue(CSSValue*& list, CSSValue*);

  static CSSValue* ConsumeBackgroundAttachment(CSSParserTokenRange&);
  static CSSValue* ConsumeBackgroundBlendMode(CSSParserTokenRange&);
  static CSSValue* ConsumeBackgroundBox(CSSParserTokenRange&);
  static CSSValue* ConsumeBackgroundComposite(CSSParserTokenRange&);
  static CSSValue* ConsumeMaskSourceType(CSSParserTokenRange&);

  static bool ConsumeBackgroundPosition(CSSParserTokenRange&,
                                        const CSSParserContext&,
                                        CSSPropertyParserHelpers::UnitlessQuirk,
                                        CSSValue*& result_x,
                                        CSSValue*& result_y);
  static CSSValue* ConsumePrefixedBackgroundBox(CSSParserTokenRange&,
                                                const CSSParserContext*,
                                                bool allow_text_value);
  static CSSValue* ConsumeBackgroundSize(CSSParserTokenRange&,
                                         CSSParserMode,
                                         bool use_legacy_parsing);
  static bool ConsumeRepeatStyleComponent(CSSParserTokenRange&,
                                          CSSValue*& value1,
                                          CSSValue*& value2,
                                          bool& implicit);
  static bool ConsumeRepeatStyle(CSSParserTokenRange&,
                                 CSSValue*& result_x,
                                 CSSValue*& result_y,
                                 bool& implicit);

  static bool ConsumeSizeProperty(CSSParserTokenRange&,
                                  CSSParserMode,
                                  bool is_prev_pos_parsed,
                                  CSSValue*&);

  static void BackgroundValuePostProcessing(CSSValue*,
                                            CSSValue* value_y,
                                            CSSValue** longhands,
                                            bool* parsed_longhand,
                                            size_t longhands_sz,
                                            size_t longhand_idx);
};

}  // namespace blink

#endif  // CSSPropertyBackgroundUtils_h
