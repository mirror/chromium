// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CSSParserImpl_h
#define CSSParserImpl_h

#include <memory>
#include "core/CSSPropertyNames.h"
#include "core/css/CSSProperty.h"
#include "core/css/CSSPropertySourceData.h"
#include "core/css/StylePropertySet.h"
#include "core/css/parser/CSSParserTokenRange.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Vector.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class CSSLazyParsingState;
class CSSParserContext;
class CSSParserObserver;
class CSSParserTokenStream;
class StyleRule;
class StyleRuleBase;
class StyleRuleCharset;
class StyleRuleFontFace;
class StyleRuleImport;
class StyleRuleKeyframe;
class StyleRuleKeyframes;
class StyleRuleMedia;
class StyleRuleNamespace;
class StyleRulePage;
class StyleRuleSupports;
class StyleRuleViewport;
class StyleSheetContents;
class Element;

class CSSParserImpl {
  STACK_ALLOCATED();
  WTF_MAKE_NONCOPYABLE(CSSParserImpl);

 public:
  CSSParserImpl(const CSSParserContext*, StyleSheetContents* = nullptr);

  enum AllowedRulesType {
    // As per css-syntax, css-cascade and css-namespaces, @charset rules
    // must come first, followed by @import then @namespace.
    // AllowImportRules actually means we allow @import and any rules thay
    // may follow it, i.e. @namespace rules and regular rules.
    // AllowCharsetRules and AllowNamespaceRules behave similarly.
    kAllowCharsetRules,
    kAllowImportRules,
    kAllowNamespaceRules,
    kRegularRules,
    kKeyframeRules,
    kApplyRules,  // For @apply inside style rules
    kNoRules,     // For parsing at-rules inside declaration lists
  };

  static MutableStylePropertySet::SetResult ParseValue(MutableStylePropertySet*,
                                                       CSSPropertyID,
                                                       const String&,
                                                       bool important,
                                                       const CSSParserContext*);
  static MutableStylePropertySet::SetResult ParseVariableValue(
      MutableStylePropertySet*,
      const AtomicString& property_name,
      const PropertyRegistry*,
      const String&,
      bool important,
      const CSSParserContext*,
      bool is_animation_tainted);
  static ImmutableStylePropertySet* ParseInlineStyleDeclaration(const String&,
                                                                Element*);
  static bool ParseDeclarationList(MutableStylePropertySet*,
                                   const String&,
                                   const CSSParserContext*);
  static StyleRuleBase* ParseRule(const String&,
                                  const CSSParserContext*,
                                  StyleSheetContents*,
                                  AllowedRulesType);
  static void ParseStyleSheet(const String&,
                              const CSSParserContext*,
                              StyleSheetContents*,
                              bool defer_property_parsing = false);
  static CSSSelectorList ParsePageSelector(const CSSParserContext&,
                                           CSSParserTokenRange,
                                           StyleSheetContents*);

  static ImmutableStylePropertySet* ParseCustomPropertySet(CSSParserTokenRange);

  static std::unique_ptr<Vector<double>> ParseKeyframeKeyList(const String&);

  bool SupportsDeclaration(CSSParserTokenRange&);

  static void ParseDeclarationListForInspector(const String&,
                                               const CSSParserContext*,
                                               CSSParserObserver&);
  static void ParseStyleSheetForInspector(const String&,
                                          const CSSParserContext*,
                                          StyleSheetContents*,
                                          CSSParserObserver&);

  static StylePropertySet* ParseDeclarationListForLazyStyle(
      const String& block,
      size_t offset,
      const CSSParserContext*);

 private:
  enum RuleListType { kTopLevelRuleList, kRegularRuleList, kKeyframesRuleList };

  // Returns whether the first encountered rule was valid
  template <typename T>
  bool ConsumeRuleList(CSSParserTokenStream&, RuleListType, T callback);

  // These two functions update the range they're given
  StyleRuleBase* ConsumeAtRule(CSSParserTokenStream&, AllowedRulesType);
  StyleRuleBase* ConsumeQualifiedRule(CSSParserTokenStream&,
                                      AllowedRulesType,
                                      size_t offset = 0);

  static StyleRuleCharset* ConsumeCharsetRule(CSSParserTokenRange prelude,
                                              size_t start_offset,
                                              size_t end_offset);
  StyleRuleImport* ConsumeImportRule(CSSParserTokenRange prelude,
                                     size_t prelude_start_offset,
                                     size_t prelude_end_offset);
  StyleRuleNamespace* ConsumeNamespaceRule(CSSParserTokenRange prelude,
                                           size_t prelude_start_offset);
  StyleRuleMedia* ConsumeMediaRule(CSSParserTokenRange prelude,
                                   CSSParserTokenStream& block,
                                   size_t prelude_start_offset);
  StyleRuleSupports* ConsumeSupportsRule(CSSParserTokenRange prelude,
                                         CSSParserTokenStream& block,
                                         size_t prelude_start_offset);
  StyleRuleViewport* ConsumeViewportRule(CSSParserTokenRange prelude,
                                         CSSParserTokenStream& block,
                                         size_t prelude_start_offset);
  StyleRuleFontFace* ConsumeFontFaceRule(CSSParserTokenRange prelude,
                                         CSSParserTokenStream& block,
                                         size_t prelude_start_offset);
  StyleRuleKeyframes* ConsumeKeyframesRule(bool webkit_prefixed,
                                           CSSParserTokenRange prelude,
                                           CSSParserTokenStream& block,
                                           size_t prelude_start_offset);
  StyleRulePage* ConsumePageRule(CSSParserTokenRange prelude,
                                 CSSParserTokenStream& block,
                                 size_t prelude_start_offset);
  // Updates parsed_properties_
  void ConsumeApplyRule(CSSParserTokenRange prelude);

  StyleRuleKeyframe* ConsumeKeyframeStyleRule(CSSParserTokenRange prelude,
                                              CSSParserTokenStream& block,
                                              size_t prelude_start_offset);
  StyleRule* ConsumeStyleRule(CSSParserTokenRange prelude,
                              CSSParserTokenStream& block,
                              size_t offset);

  void ConsumeDeclarationListForAtApply(CSSParserTokenRange);
  void ConsumeDeclarationList(CSSParserTokenStream&, StyleRule::RuleType);
  void ConsumeDeclaration(CSSParserTokenStream&,
                          StyleRule::RuleType,
                          size_t declaration_start_offset);
  void ConsumeDeclaration(CSSParserTokenRange,
                          StyleRule::RuleType,
                          size_t declaration_start_offset,
                          size_t declaration_end_offset);
  void ConsumeDeclarationValue(CSSParserTokenRange,
                               CSSPropertyID,
                               bool important,
                               StyleRule::RuleType);
  void ConsumeVariableValue(CSSParserTokenRange,
                            const AtomicString& property_name,
                            bool important,
                            bool is_animation_tainted);

  static std::unique_ptr<Vector<double>> ConsumeKeyframeKeyList(
      CSSParserTokenRange);

  // FIXME: Can we build StylePropertySets directly?
  // FIXME: Investigate using a smaller inline buffer
  HeapVector<CSSProperty, 256> parsed_properties_;

  Member<const CSSParserContext> context_;
  Member<StyleSheetContents> style_sheet_;

  Member<CSSLazyParsingState> lazy_state_;

  CSSParserObserver* observer_ = nullptr;
};

}  // namespace blink

#endif  // CSSParserImpl_h
