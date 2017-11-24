// Copyright 2016 the chromium authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/cssom/StylePropertyMapReadonly.h"

#include "bindings/core/v8/ExceptionState.h"
#include "core/CSSPropertyNames.h"
#include "core/css/CSSCustomPropertyDeclaration.h"
#include "core/css/CSSValueList.h"
#include "core/css/cssom/CSSStyleValue.h"
#include "core/css/cssom/CSSUnsupportedStyleValue.h"
#include "core/css/cssom/StyleValueFactory.h"
#include "core/css/properties/CSSProperty.h"

namespace blink {

namespace {

class StylePropertyMapIterationSource final
    : public PairIterable<String, CSSStyleValueOrCSSStyleValueSequence>::
          IterationSource {
 public:
  explicit StylePropertyMapIterationSource(
      HeapVector<StylePropertyMapReadonly::StylePropertyMapEntry> values)
      : index_(0), values_(values) {}

  bool Next(ScriptState*,
            String& key,
            CSSStyleValueOrCSSStyleValueSequence& value,
            ExceptionState&) override {
    if (index_ >= values_.size())
      return false;

    const StylePropertyMapReadonly::StylePropertyMapEntry& pair =
        values_.at(index_++);
    key = pair.first;
    value = pair.second;
    return true;
  }

  virtual void Trace(blink::Visitor* visitor) {
    visitor->Trace(values_);
    PairIterable<String, CSSStyleValueOrCSSStyleValueSequence>::
        IterationSource::Trace(visitor);
  }

 private:
  size_t index_;
  const HeapVector<StylePropertyMapReadonly::StylePropertyMapEntry> values_;
};

}  // namespace

CSSStyleValue* StylePropertyMapReadonly::get(const String& property_name,
                                             ExceptionState& exception_state) {
  CSSStyleValueVector style_vector = getAll(property_name, exception_state);
  return style_vector.IsEmpty() ? nullptr : style_vector[0];
}

CSSStyleValueVector StylePropertyMapReadonly::getAll(
    const String& property_name,
    ExceptionState& exception_state) {
  CSSPropertyID property_id = cssPropertyID(property_name);
  if (property_id == CSSPropertyInvalid) {
    exception_state.ThrowTypeError("Invalid propertyName: " + property_name);
    return CSSStyleValueVector();
  }

  DCHECK(isValidCSSPropertyID(property_id));
  const CSSValue* value = (property_id == CSSPropertyVariable)
                              ? GetCustomProperty(AtomicString(property_name))
                              : GetProperty(property_id);
  if (!value)
    return CSSStyleValueVector();

  return StyleValueFactory::CssValueToStyleValueVector(property_id, *value);
}

bool StylePropertyMapReadonly::has(const String& property_name,
                                   ExceptionState& exception_state) {
  return !getAll(property_name, exception_state).IsEmpty();
}

Vector<String> StylePropertyMapReadonly::getProperties() {
  // TODO(779841): Needs to be sorted.
  Vector<String> result;

  ForEachProperty([&result](CSSPropertyID property_id, const CSSValue& value) {
    DCHECK(isValidCSSPropertyID(property_id));
    if (property_id == CSSPropertyVariable)
      result.push_back(ToCSSCustomPropertyDeclaration(value).GetName());
    else
      result.push_back(getPropertyNameString(property_id));
  });

  return result;
}

StylePropertyMapReadonly::IterationSource*
StylePropertyMapReadonly::StartIteration(ScriptState*, ExceptionState&) {
  // TODO(779841): Needs to be sorted.
  HeapVector<StylePropertyMapReadonly::StylePropertyMapEntry> result;

  ForEachProperty([&result](CSSPropertyID property_id,
                            const CSSValue& css_value) {
    DCHECK(isValidCSSPropertyID(property_id));
    String name;
    CSSStyleValueOrCSSStyleValueSequence value;
    if (property_id == CSSPropertyVariable) {
      // TODO(meade): Eventually custom properties will support other types, so
      // actually return them instead of always returning a
      // CSSUnsupportedStyleValue.
      // TODO(779477): Should these return CSSUnparsedValues?
      const auto& custom_declaration =
          ToCSSCustomPropertyDeclaration(css_value);
      name = custom_declaration.GetName();
      value.SetCSSStyleValue(
          CSSUnsupportedStyleValue::Create(custom_declaration.CustomCSSText()));
    } else {
      name = getPropertyNameString(property_id);
      auto style_value_vector =
          StyleValueFactory::CssValueToStyleValueVector(property_id, css_value);
      if (style_value_vector.size() == 1)
        value.SetCSSStyleValue(style_value_vector[0]);
      else
        value.SetCSSStyleValueSequence(style_value_vector);
    }
    result.emplace_back(name, value);
  });

  return new StylePropertyMapIterationSource(result);
}

}  // namespace blink
