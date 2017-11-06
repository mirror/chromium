// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AtRuleDescriptorParser_h
#define AtRuleDescriptorParser_h

#include "core/css/parser/AtRuleDescriptors.h"

namespace blink {

class CSSParserContext;
class CSSValue;

class AtRuleDescriptorParser {
  STATIC_ONLY(AtRuleDescriptorParser);

 public:
  static CSSValue* ParseFontFaceDescriptor(AtRuleDescriptorID,
                                           const String&,
                                           const CSSParserContext&);
};

}  // namespace blink

#endif  // AtRuleDescriptorParser_h
