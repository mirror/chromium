// Copyright 2016 the chromium authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/cssom/StylePropertyMap.h"

#include "bindings/core/v8/ExceptionState.h"
#include "core/css/CSSValueList.h"
#include "core/css/cssom/CSSOMTypes.h"
#include "core/css/cssom/CSSStyleValue.h"
#include "core/css/cssom/StyleValueFactory.h"
#include "core/css/parser/CSSParserContext.h"
#include "core/css/properties/CSSProperty.h"

namespace blink {

namespace {

CSSValueList* CssValueListForPropertyID(CSSPropertyID property_id) {
  DCHECK(CSSProperty::Get(property_id).IsRepeated());
  char separator = CSSProperty::Get(property_id).RepetitionSeparator();
  switch (separator) {
    case ' ':
      return CSSValueList::CreateSpaceSeparated();
    case ',':
      return CSSValueList::CreateCommaSeparated();
    case '/':
      return CSSValueList::CreateSlashSeparated();
    default:
      NOTREACHED();
      return nullptr;
  }
}

const CSSValue* StyleValueToCSSValue(CSSPropertyID property_id,
                                     const CSSStyleValue& style_value,
                                     SecureContextMode secure_context_mode) {
  if (!CSSOMTypes::PropertyCanTake(property_id, style_value))
    return nullptr;
  return style_value.ToCSSValueWithProperty(property_id, secure_context_mode);
}

const CSSValue* CollectStyleValuesOrStringsAsCSSValues(
    CSSPropertyID property_id,
    const HeapVector<CSSStyleValueOrString>& values,
    const CSSParserContext* parser_context,
    CSSValueList* result) {
  DCHECK(!values.IsEmpty());

  for (const auto& value : values) {
    if (value.IsCSSStyleValue()) {
      if (!value.GetAsCSSStyleValue())
        return nullptr;

      const CSSValue* css_value =
          StyleValueToCSSValue(property_id, *value.GetAsCSSStyleValue(),
                               parser_context->GetSecureContextMode());
      if (!css_value)
        return nullptr;

      result->Append(*css_value);
    } else {
      DCHECK(value.IsString());
      const auto subvalues = StyleValueFactory::FromString(
          property_id, value.GetAsString(), parser_context);
      for (const auto& subvalue : subvalues) {
        const CSSValue* css_value = StyleValueToCSSValue(
            property_id, *subvalue, parser_context->GetSecureContextMode());
        if (!css_value)
          return nullptr;

        result->Append(*css_value);
      }
    }
  }

  if (!CSSProperty::Get(property_id).IsRepeated() ||
      std::any_of(result->begin(), result->end(), [](const auto& value) {
        return value->IsCSSWideKeyword();
      })) {
    if (result->length() > 1)
      return nullptr;
    return &result->Item(0);
  }

  return result;
}

}  // namespace

void StylePropertyMap::set(const ExecutionContext* execution_context,
                           const String& property_name,
                           const HeapVector<CSSStyleValueOrString>& values,
                           ExceptionState& exception_state) {
  if (values.IsEmpty())
    return;

  const CSSPropertyID property_id = cssPropertyID(property_name);

  if (property_id == CSSPropertyInvalid || property_id == CSSPropertyVariable) {
    // TODO(meade): Handle custom properties here.
    exception_state.ThrowTypeError("Invalid propertyName: " + property_name);
    return;
  }

  CSSValueList* initial_value_list = nullptr;
  if (CSSProperty::Get(property_id).IsRepeated()) {
    initial_value_list = CssValueListForPropertyID(property_id);
  } else {
    initial_value_list = CSSValueList::CreateCommaSeparated();
  }

  const CSSValue* result = CollectStyleValuesOrStringsAsCSSValues(
      property_id, values, CSSParserContext::Create(*execution_context),
      initial_value_list);
  if (!result) {
    exception_state.ThrowTypeError("Invalid type for property");
    return;
  }

  SetProperty(property_id, result);
}

void StylePropertyMap::append(const ExecutionContext* execution_context,
                              const String& property_name,
                              const HeapVector<CSSStyleValueOrString>& values,
                              ExceptionState& exception_state) {
  if (values.IsEmpty())
    return;

  const CSSPropertyID property_id = cssPropertyID(property_name);

  if (property_id == CSSPropertyInvalid || property_id == CSSPropertyVariable) {
    // TODO(meade): Handle custom properties here.
    exception_state.ThrowTypeError("Invalid propertyName: " + property_name);
    return;
  }

  if (!CSSProperty::Get(property_id).IsRepeated()) {
    exception_state.ThrowTypeError("Property does not support multiple values");
    return;
  }

  CSSValueList* initial_value_list = nullptr;
  if (const CSSValue* css_value = GetProperty(property_id)) {
    DCHECK(css_value->IsValueList());
    initial_value_list = ToCSSValueList(css_value)->Copy();
  } else {
    initial_value_list = CssValueListForPropertyID(property_id);
  }

  const CSSValue* result = CollectStyleValuesOrStringsAsCSSValues(
      property_id, values, CSSParserContext::Create(*execution_context),
      initial_value_list);
  if (!result) {
    exception_state.ThrowTypeError("Invalid type for property");
    return;
  }

  SetProperty(property_id, result);
}

void StylePropertyMap::remove(const String& property_name,
                              ExceptionState& exception_state) {
  CSSPropertyID property_id = cssPropertyID(property_name);

  if (property_id == CSSPropertyInvalid || property_id == CSSPropertyVariable) {
    // TODO(meade): Handle custom properties here.
    exception_state.ThrowTypeError("Invalid propertyName: " + property_name);
  }

  RemoveProperty(property_id);
}

void StylePropertyMap::update(const ExecutionContext* execution_context,
                              const String& property_name,
                              V8UpdateFunction* update_function,
                              ExceptionState& exception_state) {
  CSSStyleValue* old_value = get(property_name, exception_state);
  if (exception_state.HadException())
    return;

  const auto& new_value = update_function->Invoke(this, old_value);
  if (new_value.IsNothing())
    return;

  HeapVector<CSSStyleValueOrString> new_value_vector;
  new_value_vector.push_back(
      CSSStyleValueOrString::FromCSSStyleValue(new_value.ToChecked()));

  // FIXME(785132): We shouldn't need an execution_context here, but
  // CSSUnsupportedStyleValue currently requires parsing.
  set(execution_context, property_name, std::move(new_value_vector),
      exception_state);
}

}  // namespace blink
