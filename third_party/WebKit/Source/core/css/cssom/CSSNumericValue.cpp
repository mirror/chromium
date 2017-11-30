// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/cssom/CSSNumericValue.h"

#include "bindings/core/v8/ExceptionState.h"
#include "core/css/CSSPrimitiveValue.h"
#include "core/css/cssom/CSSMathInvert.h"
#include "core/css/cssom/CSSMathMax.h"
#include "core/css/cssom/CSSMathMin.h"
#include "core/css/cssom/CSSMathNegate.h"
#include "core/css/cssom/CSSMathProduct.h"
#include "core/css/cssom/CSSMathSum.h"
#include "core/css/cssom/CSSNumericSumValue.h"
#include "core/css/cssom/CSSUnitValue.h"

#include <numeric>

namespace blink {

namespace {

template <CSSStyleValue::StyleValueType type>
void PrependValueForArithmetic(CSSNumericValueVector& vector,
                               CSSNumericValue* value) {
  DCHECK(value);
  if (value->GetType() == type)
    vector.PrependVector(static_cast<CSSMathVariadic*>(value)->NumericValues());
  else
    vector.push_front(value);
}

template <class BinaryOperation>
CSSUnitValue* MaybeSimplifyAsUnitValue(const CSSNumericValueVector& values,
                                       const BinaryOperation& op) {
  DCHECK(!values.IsEmpty());

  CSSUnitValue* first_unit_value = ToCSSUnitValueOrNull(values[0]);
  if (!first_unit_value)
    return nullptr;

  double final_value = first_unit_value->value();
  for (size_t i = 1; i < values.size(); i++) {
    CSSUnitValue* unit_value = ToCSSUnitValueOrNull(values[i]);
    if (!unit_value ||
        unit_value->GetInternalUnit() != first_unit_value->GetInternalUnit())
      return nullptr;

    final_value = op(final_value, unit_value->value());
  }

  return CSSUnitValue::Create(final_value, first_unit_value->GetInternalUnit());
}

CSSUnitValue* CSSNumericSumValueEntryToUnitValue(
    const CSSNumericSumValue::Term& term) {
  if (term.units.size() == 0)
    return CSSUnitValue::Create(term.value);
  if (term.units.size() == 1 && term.units.begin()->value == 1)
    return CSSUnitValue::Create(term.value, term.units.begin()->key);
  return nullptr;
}

}  // namespace

bool CSSNumericValue::IsValidUnit(CSSPrimitiveValue::UnitType unit) {
  // UserUnits returns true for CSSPrimitiveValue::IsLength below.
  if (unit == CSSPrimitiveValue::UnitType::kUserUnits)
    return false;
  if (unit == CSSPrimitiveValue::UnitType::kNumber ||
      unit == CSSPrimitiveValue::UnitType::kPercentage ||
      CSSPrimitiveValue::IsLength(unit) || CSSPrimitiveValue::IsAngle(unit) ||
      CSSPrimitiveValue::IsTime(unit) || CSSPrimitiveValue::IsFrequency(unit) ||
      CSSPrimitiveValue::IsResolution(unit) || CSSPrimitiveValue::IsFlex(unit))
    return true;
  return false;
}

CSSPrimitiveValue::UnitType CSSNumericValue::UnitFromName(const String& name) {
  if (EqualIgnoringASCIICase(name, "number"))
    return CSSPrimitiveValue::UnitType::kNumber;
  if (EqualIgnoringASCIICase(name, "percent") || name == "%")
    return CSSPrimitiveValue::UnitType::kPercentage;
  return CSSPrimitiveValue::StringToUnitType(name);
}

CSSNumericValue* CSSNumericValue::parse(const String& css_text,
                                        ExceptionState&) {
  // TODO(meade): Implement
  return nullptr;
}

CSSNumericValue* CSSNumericValue::FromCSSValue(const CSSPrimitiveValue& value) {
  if (value.IsCalculated()) {
    // TODO(meade): Implement this case.
    return nullptr;
  }
  return CSSUnitValue::FromCSSValue(value);
}

/* static */
CSSNumericValue* CSSNumericValue::FromNumberish(const CSSNumberish& value) {
  if (value.IsDouble()) {
    return CSSUnitValue::Create(value.GetAsDouble(),
                                CSSPrimitiveValue::UnitType::kNumber);
  }
  return value.GetAsCSSNumericValue();
}

CSSNumericValue* CSSNumericValue::to(const String& unit_string,
                                     ExceptionState& exception_state) {
  CSSPrimitiveValue::UnitType target_unit = UnitFromName(unit_string);
  if (!IsValidUnit(target_unit)) {
    exception_state.ThrowDOMException(kSyntaxError,
                                      "Invalid unit for conversion");
    return nullptr;
  }

  CSSNumericValue* result = to(target_unit);
  if (!result) {
    exception_state.ThrowTypeError("Cannot convert to " + unit_string);
    return nullptr;
  }

  return result;
}

CSSUnitValue* CSSNumericValue::to(CSSPrimitiveValue::UnitType unit) const {
  const auto sum = SumValue();
  if (!sum || sum->terms.size() != 1)
    return nullptr;

  CSSUnitValue* value = CSSNumericSumValueEntryToUnitValue(sum->terms[0]);
  if (!value)
    return nullptr;
  return value->ConvertTo(unit);
}

CSSMathSum* CSSNumericValue::toSum(const Vector<String>& unit_strings,
                                   ExceptionState& exception_state) {
  for (const auto& unit_string : unit_strings) {
    if (!IsValidUnit(UnitFromName(unit_string))) {
      exception_state.ThrowDOMException(kSyntaxError,
                                        "Invalid unit for conversion");
      return nullptr;
    }
  }

  const auto sum = SumValue();
  if (!sum) {
    exception_state.ThrowTypeError("Invalid value for conversion");
    return nullptr;
  }

  CSSNumericValueVector values;
  for (const auto& term : sum->terms) {
    CSSUnitValue* value = CSSNumericSumValueEntryToUnitValue(term);
    if (!value) {
      exception_state.ThrowTypeError("Invalid value for conversion");
      return nullptr;
    }
    values.push_back(value);
  }

  if (unit_strings.size() == 0) {
    std::sort(values.begin(), values.end(), [](const auto& a, const auto& b) {
      return WTF::CodePointCompareLessThan(ToCSSUnitValue(a)->unit(),
                                           ToCSSUnitValue(b)->unit());
    });

    // We got 'values' from a sum value, so it must be a valid CSSMathSum.
    auto* result = CSSMathSum::Create(values);
    DCHECK(result);
    return result;
  }

  CSSNumericValueVector result;
  for (const auto& unit_string : unit_strings) {
    CSSPrimitiveValue::UnitType target_unit = UnitFromName(unit_string);
    DCHECK(IsValidUnit(target_unit));

    // Collect all the terms that are compatible with this unit.
    // We mark used terms as null so we don't use them again.
    double total_value =
        std::accumulate(values.begin(), values.end(), 0.0,
                        [target_unit](double cur_sum, auto& value) {
                          if (value) {
                            auto& unit_value = ToCSSUnitValue(*value);
                            if (const auto* converted_value =
                                    unit_value.ConvertTo(target_unit)) {
                              cur_sum += converted_value->value();
                              value = nullptr;
                            }
                          }
                          return cur_sum;
                        });

    result.push_back(CSSUnitValue::Create(total_value, target_unit));
  }

  if (std::any_of(values.begin(), values.end(),
                  [](const auto& v) { return v; })) {
    exception_state.ThrowTypeError(
        "There were leftover terms that were not converted");
    return nullptr;
  }

  auto* value = CSSMathSum::Create(result);
  if (!value) {
    exception_state.ThrowTypeError("Can't create CSSMathSum");
    return nullptr;
  }
  return value;
}

CSSNumericValue* CSSNumericValue::add(
    const HeapVector<CSSNumberish>& numberishes,
    ExceptionState& exception_state) {
  auto values = CSSNumberishesToNumericValues(numberishes);
  PrependValueForArithmetic<kSumType>(values, this);

  if (CSSUnitValue* unit_value =
          MaybeSimplifyAsUnitValue(values, std::plus<double>())) {
    return unit_value;
  }
  return CSSMathSum::Create(std::move(values));
}

CSSNumericValue* CSSNumericValue::sub(
    const HeapVector<CSSNumberish>& numberishes,
    ExceptionState& exception_state) {
  auto values = CSSNumberishesToNumericValues(numberishes);
  std::transform(values.begin(), values.end(), values.begin(),
                 [](CSSNumericValue* v) { return v->Negate(); });
  PrependValueForArithmetic<kSumType>(values, this);

  if (CSSUnitValue* unit_value =
          MaybeSimplifyAsUnitValue(values, std::plus<double>())) {
    return unit_value;
  }
  return CSSMathSum::Create(std::move(values));
}

CSSNumericValue* CSSNumericValue::mul(
    const HeapVector<CSSNumberish>& numberishes,
    ExceptionState& exception_state) {
  auto values = CSSNumberishesToNumericValues(numberishes);
  PrependValueForArithmetic<kProductType>(values, this);

  if (CSSUnitValue* unit_value =
          MaybeSimplifyAsUnitValue(values, std::multiplies<double>())) {
    return unit_value;
  }
  return CSSMathProduct::Create(std::move(values));
}

CSSNumericValue* CSSNumericValue::div(
    const HeapVector<CSSNumberish>& numberishes,
    ExceptionState& exception_state) {
  auto values = CSSNumberishesToNumericValues(numberishes);
  std::transform(values.begin(), values.end(), values.begin(),
                 [](CSSNumericValue* v) { return v->Invert(); });
  PrependValueForArithmetic<kProductType>(values, this);

  if (CSSUnitValue* unit_value =
          MaybeSimplifyAsUnitValue(values, std::multiplies<double>())) {
    return unit_value;
  }
  return CSSMathProduct::Create(std::move(values));
}

CSSNumericValue* CSSNumericValue::min(
    const HeapVector<CSSNumberish>& numberishes,
    ExceptionState& exception_state) {
  auto values = CSSNumberishesToNumericValues(numberishes);
  PrependValueForArithmetic<kMinType>(values, this);

  if (CSSUnitValue* unit_value = MaybeSimplifyAsUnitValue(
          values, [](double a, double b) { return std::min(a, b); })) {
    return unit_value;
  }
  return CSSMathMin::Create(std::move(values));
}

CSSNumericValue* CSSNumericValue::max(
    const HeapVector<CSSNumberish>& numberishes,
    ExceptionState& exception_state) {
  auto values = CSSNumberishesToNumericValues(numberishes);
  PrependValueForArithmetic<kMaxType>(values, this);

  if (CSSUnitValue* unit_value = MaybeSimplifyAsUnitValue(
          values, [](double a, double b) { return std::max(a, b); })) {
    return unit_value;
  }
  return CSSMathMax::Create(std::move(values));
}

bool CSSNumericValue::equals(const HeapVector<CSSNumberish>& args) {
  CSSNumericValueVector values = CSSNumberishesToNumericValues(args);
  return std::all_of(values.begin(), values.end(),
                     [this](const auto& v) { return this->Equals(*v); });
}

CSSNumericValue* CSSNumericValue::Negate() {
  return CSSMathNegate::Create(this);
}

CSSNumericValue* CSSNumericValue::Invert() {
  return CSSMathInvert::Create(this);
}

CSSNumericValueVector CSSNumberishesToNumericValues(
    const HeapVector<CSSNumberish>& values) {
  CSSNumericValueVector result;
  for (const CSSNumberish& value : values) {
    result.push_back(CSSNumericValue::FromNumberish(value));
  }
  return result;
}

}  // namespace blink
