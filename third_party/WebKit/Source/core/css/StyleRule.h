/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * (C) 2002-2003 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2002, 2006, 2008, 2012, 2013 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef StyleRule_h
#define StyleRule_h

#include "core/CoreExport.h"
#include "core/css/CSSPropertyValueSet.h"
#include "core/css/CSSSelectorList.h"
#include "core/css/MediaList.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/RefPtr.h"

namespace blink {

class CSSRule;
class CSSStyleSheet;

class CORE_EXPORT StyleRuleBase
    : public GarbageCollectedFinalized<StyleRuleBase> {
 public:
  enum RuleType {
    kCharset,
    kStyle,
    kImport,
    kMedia,
    kFontFace,
    kPage,
    kKeyframes,
    kKeyframe,
    kNamespace,
    kSupports,
    kViewport,
  };

  virtual bool IsCharsetRule() const { return false; }
  virtual bool IsFontFaceRule() const { return false; }
  virtual bool IsKeyframesRule() const { return false; }
  virtual bool IsKeyframeRule() const { return false; }
  virtual bool IsNamespaceRule() const { return false; }
  virtual bool IsMediaRule() const { return false; }
  virtual bool IsPageRule() const { return false; }
  virtual bool IsStyleRule() const { return false; }
  virtual bool IsSupportsRule() const { return false; }
  virtual bool IsViewportRule() const { return false; }
  virtual bool IsImportRule() const { return false; }

  virtual StyleRuleBase* Copy() const {
    NOTREACHED();
    return nullptr;
  }

  // FIXME: There shouldn't be any need for the null parent version.
  CSSRule* CreateCSSOMWrapper(CSSStyleSheet* parent_sheet = nullptr) const;
  CSSRule* CreateCSSOMWrapper(CSSRule* parent_rule) const;

  virtual bool HasFailedOrCanceledSubresources() const { return false; }

  virtual void Trace(blink::Visitor*) {}

  // ~StyleRuleBase should be public, because non-public ~StyleRuleBase
  // causes C2248 error : 'blink::StyleRuleBase::~StyleRuleBase' : cannot
  // access protected member declared in class 'blink::StyleRuleBase' when
  // compiling 'source\wtf\refcounted.h' by using msvc.
  virtual ~StyleRuleBase() {}

 protected:
  StyleRuleBase() {}

 private:
  CSSRule* CreateCSSOMWrapper(CSSStyleSheet* parent_sheet,
                              CSSRule* parent_rule) const;
  virtual CSSRule* CreateCSSOMWrapperInternal(
      CSSStyleSheet* parent_sheet) const {
    NOTREACHED();
    return nullptr;
  }
};

class CORE_EXPORT StyleRule : public StyleRuleBase {
 public:
  // Adopts the selector list
  static StyleRule* Create(CSSSelectorList selector_list,
                           CSSPropertyValueSet* properties) {
    return new StyleRule(std::move(selector_list), properties);
  }
  static StyleRule* CreateLazy(CSSSelectorList selector_list,
                               CSSLazyPropertyParser* lazy_property_parser) {
    return new StyleRule(std::move(selector_list), lazy_property_parser);
  }

  ~StyleRule() override;

  const CSSSelectorList& SelectorList() const { return selector_list_; }
  const CSSPropertyValueSet& Properties() const;
  MutableCSSPropertyValueSet& MutableProperties();

  void WrapperAdoptSelectorList(CSSSelectorList selectors) {
    selector_list_ = std::move(selectors);
  }

  StyleRule* Copy() const override { return new StyleRule(*this); }

  static unsigned AverageSizeInBytes();

  // Helper methods to avoid parsing lazy properties when not needed.
  bool HasFailedOrCanceledSubresources() const override;
  bool ShouldConsiderForMatchingRules(bool include_empty_rules) const;

  bool IsStyleRule() const override { return true; }
  void Trace(blink::Visitor*) override;

 private:
  friend class CSSLazyParsingTest;

  StyleRule(CSSSelectorList, CSSPropertyValueSet*);
  StyleRule(CSSSelectorList, CSSLazyPropertyParser*);
  StyleRule(const StyleRule&);

  bool HasParsedProperties() const;
  CSSRule* CreateCSSOMWrapperInternal(
      CSSStyleSheet* parent_sheet) const override;

  // Whether or not we should consider this for matching rules. Usually we try
  // to avoid considering empty property sets, as an optimization. This is
  // not possible for lazy properties, which always need to be considered. The
  // lazy parser does its best to avoid lazy parsing for properties that look
  // empty due to lack of tokens.
  enum ConsiderForMatching {
    kAlwaysConsider,
    kConsiderIfNonEmpty,
  };
  mutable ConsiderForMatching should_consider_for_matching_rules_;

  CSSSelectorList selector_list_;
  mutable Member<CSSPropertyValueSet> properties_;
  mutable Member<CSSLazyPropertyParser> lazy_property_parser_;
};

class CORE_EXPORT StyleRuleFontFace : public StyleRuleBase {
 public:
  static StyleRuleFontFace* Create(CSSPropertyValueSet* properties) {
    return new StyleRuleFontFace(properties);
  }

  ~StyleRuleFontFace() override;

  const CSSPropertyValueSet& Properties() const { return *properties_; }
  MutableCSSPropertyValueSet& MutableProperties();

  StyleRuleFontFace* Copy() const override {
    return new StyleRuleFontFace(*this);
  }

  bool HasFailedOrCanceledSubresources() const override {
    return properties_->HasFailedOrCanceledSubresources();
  }
  bool IsFontFaceRule() const override { return true; }
  void Trace(blink::Visitor*) override;

 private:
  StyleRuleFontFace(CSSPropertyValueSet*);
  StyleRuleFontFace(const StyleRuleFontFace&);
  CSSRule* CreateCSSOMWrapperInternal(
      CSSStyleSheet* parent_sheet) const override;

  Member<CSSPropertyValueSet> properties_;  // Cannot be null.
};

class StyleRulePage : public StyleRuleBase {
 public:
  // Adopts the selector list
  static StyleRulePage* Create(CSSSelectorList selector_list,
                               CSSPropertyValueSet* properties) {
    return new StyleRulePage(std::move(selector_list), properties);
  }

  ~StyleRulePage() override;

  const CSSSelector* Selector() const { return selector_list_.First(); }
  const CSSPropertyValueSet& Properties() const { return *properties_; }
  MutableCSSPropertyValueSet& MutableProperties();

  void WrapperAdoptSelectorList(CSSSelectorList selectors) {
    selector_list_ = std::move(selectors);
  }

  StyleRulePage* Copy() const override { return new StyleRulePage(*this); }

  bool IsPageRule() const override { return true; }
  void Trace(blink::Visitor*) override;

 private:
  StyleRulePage(CSSSelectorList, CSSPropertyValueSet*);
  StyleRulePage(const StyleRulePage&);
  CSSRule* CreateCSSOMWrapperInternal(
      CSSStyleSheet* parent_sheet) const override;

  Member<CSSPropertyValueSet> properties_;  // Cannot be null.
  CSSSelectorList selector_list_;
};

class CORE_EXPORT StyleRuleGroup : public StyleRuleBase {
 public:
  const HeapVector<Member<StyleRuleBase>>& ChildRules() const {
    return child_rules_;
  }

  void WrapperInsertRule(unsigned, StyleRuleBase*);
  void WrapperRemoveRule(unsigned);

  bool IsCharsetRule() const override { return type_ == kCharset; }
  bool IsFontFaceRule() const override { return type_ == kFontFace; }
  bool IsKeyframesRule() const override { return type_ == kKeyframes; }
  bool IsKeyframeRule() const override { return type_ == kKeyframe; }
  bool IsNamespaceRule() const override { return type_ == kNamespace; }
  bool IsMediaRule() const override { return type_ == kMedia; }
  bool IsPageRule() const override { return type_ == kPage; }
  bool IsStyleRule() const override { return type_ == kStyle; }
  bool IsSupportsRule() const override { return type_ == kSupports; }
  bool IsViewportRule() const override { return type_ == kViewport; }
  bool IsImportRule() const override { return type_ == kImport; }
  void Trace(blink::Visitor*) override;

 protected:
  StyleRuleGroup(RuleType, HeapVector<Member<StyleRuleBase>>& adopt_rule);
  StyleRuleGroup(const StyleRuleGroup&);

 private:
  RuleType type_;
  HeapVector<Member<StyleRuleBase>> child_rules_;
};

class CORE_EXPORT StyleRuleCondition : public StyleRuleGroup {
 public:
  String ConditionText() const { return condition_text_; }

  void Trace(blink::Visitor* visitor) override {
    StyleRuleGroup::Trace(visitor);
  }

 protected:
  StyleRuleCondition(RuleType, HeapVector<Member<StyleRuleBase>>& adopt_rule);
  StyleRuleCondition(RuleType,
                     const String& condition_text,
                     HeapVector<Member<StyleRuleBase>>& adopt_rule);
  StyleRuleCondition(const StyleRuleCondition&);

  String condition_text_;
};

class CORE_EXPORT StyleRuleMedia : public StyleRuleCondition {
 public:
  static StyleRuleMedia* Create(
      scoped_refptr<MediaQuerySet> media,
      HeapVector<Member<StyleRuleBase>>& adopt_rules) {
    return new StyleRuleMedia(media, adopt_rules);
  }

  MediaQuerySet* MediaQueries() const { return media_queries_.get(); }

  StyleRuleMedia* Copy() const override { return new StyleRuleMedia(*this); }

  bool IsMediaRule() const override { return true; }
  void Trace(blink::Visitor*) override;

 private:
  StyleRuleMedia(scoped_refptr<MediaQuerySet>,
                 HeapVector<Member<StyleRuleBase>>& adopt_rules);
  StyleRuleMedia(const StyleRuleMedia&);

  CSSRule* CreateCSSOMWrapperInternal(
      CSSStyleSheet* parent_sheet) const override;

  scoped_refptr<MediaQuerySet> media_queries_;
};

class StyleRuleSupports : public StyleRuleCondition {
 public:
  static StyleRuleSupports* Create(
      const String& condition_text,
      bool condition_is_supported,
      HeapVector<Member<StyleRuleBase>>& adopt_rules) {
    return new StyleRuleSupports(condition_text, condition_is_supported,
                                 adopt_rules);
  }

  bool ConditionIsSupported() const { return condition_is_supported_; }
  StyleRuleSupports* Copy() const override {
    return new StyleRuleSupports(*this);
  }

  bool IsSupportsRule() const override { return true; }
  void Trace(blink::Visitor* visitor) override {
    StyleRuleCondition::Trace(visitor);
  }

 private:
  StyleRuleSupports(const String& condition_text,
                    bool condition_is_supported,
                    HeapVector<Member<StyleRuleBase>>& adopt_rules);
  StyleRuleSupports(const StyleRuleSupports&);

  CSSRule* CreateCSSOMWrapperInternal(
      CSSStyleSheet* parent_sheet) const override;

  String condition_text_;
  bool condition_is_supported_;
};

class StyleRuleViewport : public StyleRuleBase {
 public:
  static StyleRuleViewport* Create(CSSPropertyValueSet* properties) {
    return new StyleRuleViewport(properties);
  }

  ~StyleRuleViewport();

  const CSSPropertyValueSet& Properties() const { return *properties_; }
  MutableCSSPropertyValueSet& MutableProperties();

  StyleRuleViewport* Copy() const override {
    return new StyleRuleViewport(*this);
  }

  bool IsViewportRule() const override { return true; }
  void Trace(blink::Visitor*) override;

 private:
  StyleRuleViewport(CSSPropertyValueSet*);
  StyleRuleViewport(const StyleRuleViewport&);

  CSSRule* CreateCSSOMWrapperInternal(
      CSSStyleSheet* parent_sheet) const override;
  Member<CSSPropertyValueSet> properties_;  // Cannot be null
};

// This should only be used within the CSS Parser
class StyleRuleCharset : public StyleRuleBase {
 public:
  static StyleRuleCharset* Create() { return new StyleRuleCharset(); }
  void Trace(blink::Visitor* visitor) override {
    StyleRuleBase::Trace(visitor);
  }

  bool IsCharsetRule() const override { return true; }

 private:
  StyleRuleCharset() {}
};

#define DEFINE_STYLE_RULE_TYPE_CASTS(Type)                \
  DEFINE_TYPE_CASTS(StyleRule##Type, StyleRuleBase, rule, \
                    rule->Is##Type##Rule(), rule.Is##Type##Rule())

DEFINE_TYPE_CASTS(StyleRule,
                  StyleRuleBase,
                  rule,
                  rule->IsStyleRule(),
                  rule.IsStyleRule());
DEFINE_STYLE_RULE_TYPE_CASTS(FontFace);
DEFINE_STYLE_RULE_TYPE_CASTS(Page);
DEFINE_STYLE_RULE_TYPE_CASTS(Media);
DEFINE_STYLE_RULE_TYPE_CASTS(Supports);
DEFINE_STYLE_RULE_TYPE_CASTS(Viewport);
DEFINE_STYLE_RULE_TYPE_CASTS(Charset);

}  // namespace blink

#endif  // StyleRule_h
