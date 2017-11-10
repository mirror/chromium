/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * (C) 2002-2003 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2002, 2005, 2006, 2008, 2012 Apple Inc. All rights reserved.
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

#include "core/css/StyleRule.h"

#include "core/css/CSSFontFaceRule.h"
#include "core/css/CSSKeyframesRule.h"
#include "core/css/CSSMediaRule.h"
#include "core/css/CSSPageRule.h"
#include "core/css/CSSStyleRule.h"
#include "core/css/CSSSupportsRule.h"
#include "core/css/CSSViewportRule.h"
#include "core/css/StyleRuleKeyframe.h"
#include "core/css/StyleRuleNamespace.h"

namespace blink {

struct SameSizeAsStyleRuleBase
    : public GarbageCollectedFinalized<SameSizeAsStyleRuleBase> {
  void* vtable;
};

static_assert(sizeof(StyleRuleBase) <= sizeof(SameSizeAsStyleRuleBase),
              "StyleRuleBase should stay small");

CSSRule* StyleRuleBase::CreateCSSOMWrapper(CSSStyleSheet* parent_sheet) const {
  return CreateCSSOMWrapper(parent_sheet, nullptr);
}

CSSRule* StyleRuleBase::CreateCSSOMWrapper(CSSRule* parent_rule) const {
  return CreateCSSOMWrapper(nullptr, parent_rule);
}

CSSRule* StyleRuleBase::CreateCSSOMWrapper(CSSStyleSheet* parent_sheet,
                                           CSSRule* parent_rule) const {
  CSSRule* rule = CreateCSSOMWrapperInternal(parent_sheet);
  if (parent_rule && rule)
    rule->SetParentRule(parent_rule);
  return rule;
}

unsigned StyleRule::AverageSizeInBytes() {
  return sizeof(StyleRule) + sizeof(CSSSelector) +
         CSSPropertyValueSet::AverageSizeInBytes();
}

StyleRule::StyleRule(CSSSelectorList selector_list,
                     CSSPropertyValueSet* properties)
    : should_consider_for_matching_rules_(kConsiderIfNonEmpty),
      selector_list_(std::move(selector_list)),
      properties_(properties) {}

StyleRule::StyleRule(CSSSelectorList selector_list,
                     CSSLazyPropertyParser* lazy_property_parser)
    : should_consider_for_matching_rules_(kAlwaysConsider),
      selector_list_(std::move(selector_list)),
      lazy_property_parser_(lazy_property_parser) {}

const CSSPropertyValueSet& StyleRule::Properties() const {
  if (!properties_) {
    properties_ = lazy_property_parser_->ParseProperties();
    lazy_property_parser_.Clear();
  }
  return *properties_;
}

StyleRule::StyleRule(const StyleRule& o)
    : should_consider_for_matching_rules_(kConsiderIfNonEmpty),
      selector_list_(o.selector_list_.Copy()),
      properties_(o.Properties().MutableCopy()) {}

StyleRule::~StyleRule() {}

MutableCSSPropertyValueSet& StyleRule::MutableProperties() {
  // Ensure properties_ is initialized.
  if (!Properties().IsMutable())
    properties_ = properties_->MutableCopy();
  return *ToMutableCSSPropertyValueSet(properties_.Get());
}

bool StyleRule::HasFailedOrCanceledSubresources() const {
  return properties_ && properties_->HasFailedOrCanceledSubresources();
}

bool StyleRule::ShouldConsiderForMatchingRules(bool include_empty_rules) const {
  DCHECK(should_consider_for_matching_rules_ == kAlwaysConsider || properties_);
  return include_empty_rules ||
         should_consider_for_matching_rules_ == kAlwaysConsider ||
         !properties_->IsEmpty();
}

void StyleRule::Trace(blink::Visitor* visitor) {
  visitor->Trace(properties_);
  visitor->Trace(lazy_property_parser_);
  StyleRuleBase::Trace(visitor);
}

bool StyleRule::HasParsedProperties() const {
  // StyleRule should only have one of {lazy_property_parser_, properties_} set.
  DCHECK(lazy_property_parser_ || properties_);
  DCHECK(!lazy_property_parser_ || !properties_);
  return !lazy_property_parser_;
}

CSSRule* StyleRule::CreateCSSOMWrapperInternal(
    CSSStyleSheet* parent_sheet) const {
  return CSSStyleRule::Create(const_cast<StyleRule*>(this), parent_sheet);
}

StyleRulePage::StyleRulePage(CSSSelectorList selector_list,
                             CSSPropertyValueSet* properties)
    : properties_(properties), selector_list_(std::move(selector_list)) {}

StyleRulePage::StyleRulePage(const StyleRulePage& page_rule)
    : properties_(page_rule.properties_->MutableCopy()),
      selector_list_(page_rule.selector_list_.Copy()) {}

StyleRulePage::~StyleRulePage() {}

MutableCSSPropertyValueSet& StyleRulePage::MutableProperties() {
  if (!properties_->IsMutable())
    properties_ = properties_->MutableCopy();
  return *ToMutableCSSPropertyValueSet(properties_.Get());
}

void StyleRulePage::Trace(blink::Visitor* visitor) {
  visitor->Trace(properties_);
  StyleRuleBase::Trace(visitor);
}

CSSRule* StyleRulePage::CreateCSSOMWrapperInternal(
    CSSStyleSheet* parent_sheet) const {
  return CSSPageRule::Create(const_cast<StyleRulePage*>(this), parent_sheet);
}

StyleRuleFontFace::StyleRuleFontFace(CSSPropertyValueSet* properties)
    : properties_(properties) {}

StyleRuleFontFace::StyleRuleFontFace(const StyleRuleFontFace& font_face_rule)
    : properties_(font_face_rule.properties_->MutableCopy()) {}

StyleRuleFontFace::~StyleRuleFontFace() {}

MutableCSSPropertyValueSet& StyleRuleFontFace::MutableProperties() {
  if (!properties_->IsMutable())
    properties_ = properties_->MutableCopy();
  return *ToMutableCSSPropertyValueSet(properties_);
}

void StyleRuleFontFace::Trace(blink::Visitor* visitor) {
  visitor->Trace(properties_);
  StyleRuleBase::Trace(visitor);
}

CSSRule* StyleRuleFontFace::CreateCSSOMWrapperInternal(
    CSSStyleSheet* parent_sheet) const {
  return CSSFontFaceRule::Create(const_cast<StyleRuleFontFace*>(this),
                                 parent_sheet);
}

StyleRuleGroup::StyleRuleGroup(RuleType type,
                               HeapVector<Member<StyleRuleBase>>& adopt_rule)
    : type_(type) {
  child_rules_.swap(adopt_rule);
}

StyleRuleGroup::StyleRuleGroup(const StyleRuleGroup& group_rule)
    : child_rules_(group_rule.child_rules_.size()) {
  for (unsigned i = 0; i < child_rules_.size(); ++i)
    child_rules_[i] = group_rule.child_rules_[i]->Copy();
}

void StyleRuleGroup::WrapperInsertRule(unsigned index, StyleRuleBase* rule) {
  child_rules_.insert(index, rule);
}

void StyleRuleGroup::WrapperRemoveRule(unsigned index) {
  child_rules_.EraseAt(index);
}

void StyleRuleGroup::Trace(blink::Visitor* visitor) {
  visitor->Trace(child_rules_);
  StyleRuleBase::Trace(visitor);
}

StyleRuleCondition::StyleRuleCondition(
    RuleType type,
    HeapVector<Member<StyleRuleBase>>& adopt_rules)
    : StyleRuleGroup(type, adopt_rules) {}

StyleRuleCondition::StyleRuleCondition(
    RuleType type,
    const String& condition_text,
    HeapVector<Member<StyleRuleBase>>& adopt_rules)
    : StyleRuleGroup(type, adopt_rules), condition_text_(condition_text) {}

StyleRuleCondition::StyleRuleCondition(const StyleRuleCondition& condition_rule)
    : StyleRuleGroup(condition_rule),
      condition_text_(condition_rule.condition_text_) {}

StyleRuleMedia::StyleRuleMedia(scoped_refptr<MediaQuerySet> media,
                               HeapVector<Member<StyleRuleBase>>& adopt_rules)
    : StyleRuleCondition(kMedia, adopt_rules), media_queries_(media) {}

StyleRuleMedia::StyleRuleMedia(const StyleRuleMedia& media_rule)
    : StyleRuleCondition(media_rule) {
  if (media_rule.media_queries_)
    media_queries_ = media_rule.media_queries_->Copy();
}

void StyleRuleMedia::Trace(blink::Visitor* visitor) {
  StyleRuleCondition::Trace(visitor);
}

CSSRule* StyleRuleMedia::CreateCSSOMWrapperInternal(
    CSSStyleSheet* parent_sheet) const {
  return CSSMediaRule::Create(const_cast<StyleRuleMedia*>(this), parent_sheet);
}

StyleRuleSupports::StyleRuleSupports(
    const String& condition_text,
    bool condition_is_supported,
    HeapVector<Member<StyleRuleBase>>& adopt_rules)
    : StyleRuleCondition(kSupports, condition_text, adopt_rules),
      condition_is_supported_(condition_is_supported) {}

StyleRuleSupports::StyleRuleSupports(const StyleRuleSupports& supports_rule)
    : StyleRuleCondition(supports_rule),
      condition_is_supported_(supports_rule.condition_is_supported_) {}

CSSRule* StyleRuleSupports::CreateCSSOMWrapperInternal(
    CSSStyleSheet* parent_sheet) const {
  return CSSSupportsRule::Create(const_cast<StyleRuleSupports*>(this),
                                 parent_sheet);
}

StyleRuleViewport::StyleRuleViewport(CSSPropertyValueSet* properties)
    : properties_(properties) {}

StyleRuleViewport::StyleRuleViewport(const StyleRuleViewport& viewport_rule)
    : properties_(viewport_rule.properties_->MutableCopy()) {}

StyleRuleViewport::~StyleRuleViewport() {}

MutableCSSPropertyValueSet& StyleRuleViewport::MutableProperties() {
  if (!properties_->IsMutable())
    properties_ = properties_->MutableCopy();
  return *ToMutableCSSPropertyValueSet(properties_);
}

void StyleRuleViewport::Trace(blink::Visitor* visitor) {
  visitor->Trace(properties_);
  StyleRuleBase::Trace(visitor);
}

CSSRule* StyleRuleViewport::CreateCSSOMWrapperInternal(
    CSSStyleSheet* parent_sheet) const {
  return CSSViewportRule::Create(const_cast<StyleRuleViewport*>(this),
                                 parent_sheet);
}

}  // namespace blink
