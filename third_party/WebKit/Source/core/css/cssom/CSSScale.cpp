// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/cssom/CSSScale.h"

#include "core/css/CSSPrimitiveValue.h"
#include "core/css/cssom/CSSNumericValue.h"

namespace blink {

namespace {

CSSScale* FromScale(const CSSFunctionValue& value) {
  DCHECK(value.length() == 1U || value.length() == 2U);
  double x = ToCSSPrimitiveValue(value.Item(0)).GetDoubleValue();
  if (value.length() == 1U) {
    return CSSScale::Create(
        CSSUnitValue::Create(x, CSSPrimitiveValue::UnitType::kNumber),
        CSSUnitValue::Create(x, CSSPrimitiveValue::UnitType::kNumber));
  }

  double y = ToCSSPrimitiveValue(value.Item(1)).GetDoubleValue();
  return CSSScale::Create(
      CSSUnitValue::Create(x, CSSPrimitiveValue::UnitType::kNumber),
      CSSUnitValue::Create(y, CSSPrimitiveValue::UnitType::kNumber));
}

CSSScale* FromScaleXYZ(const CSSFunctionValue& value) {
  DCHECK_EQ(value.length(), 1U);

  CSSNumericValue* numeric_value =
      CSSUnitValue::Create(ToCSSPrimitiveValue(value.Item(0)).GetDoubleValue(),
                           CSSPrimitiveValue::UnitType::kNumber);
  CSSUnitValue* default_value =
      CSSUnitValue::Create(1, CSSPrimitiveValue::UnitType::kNumber);
  switch (value.FunctionType()) {
    case CSSValueScaleX:
      return CSSScale::Create(numeric_value, default_value);
    case CSSValueScaleY:
      return CSSScale::Create(default_value, numeric_value);
    case CSSValueScaleZ:
      return CSSScale::Create(default_value, default_value, numeric_value);
    default:
      NOTREACHED();
      return nullptr;
  }
}

CSSScale* FromScale3d(const CSSFunctionValue& value) {
  DCHECK_EQ(value.length(), 3U);
  double x = ToCSSPrimitiveValue(value.Item(0)).GetDoubleValue();
  double y = ToCSSPrimitiveValue(value.Item(1)).GetDoubleValue();
  double z = ToCSSPrimitiveValue(value.Item(2)).GetDoubleValue();

  return CSSScale::Create(
      CSSUnitValue::Create(x, CSSPrimitiveValue::UnitType::kNumber),
      CSSUnitValue::Create(y, CSSPrimitiveValue::UnitType::kNumber),
      CSSUnitValue::Create(z, CSSPrimitiveValue::UnitType::kNumber));
}

}  // namespace

CSSScale* CSSScale::Create(const CSSNumberish& x,
                           const CSSNumberish& y,
                           ExceptionState& exception_state) {
  CSSNumericValue* x_value = CSSNumericValue::FromNumberish(x);
  CSSNumericValue* y_value = CSSNumericValue::FromNumberish(y);

  if (!x_value->IsUnitValue() || !y_value->IsUnitValue()) {
    exception_state.ThrowTypeError("CSSMathValue are not supported yet");
    return nullptr;
  }

  if (x_value->GetType() != CSSStyleValue::StyleValueType::kNumberType ||
      y_value->GetType() != CSSStyleValue::StyleValueType::kNumberType) {
    exception_state.ThrowTypeError("CSSScale does not support non-number type");
    return nullptr;
  }

  return new CSSScale(x_value, y_value);
}

CSSScale* CSSScale::Create(const CSSNumberish& x,
                           const CSSNumberish& y,
                           const CSSNumberish& z,
                           ExceptionState& exception_state) {
  CSSNumericValue* x_value = CSSNumericValue::FromNumberish(x);
  CSSNumericValue* y_value = CSSNumericValue::FromNumberish(y);
  CSSNumericValue* z_value = CSSNumericValue::FromNumberish(z);

  if (!x_value->IsUnitValue() || !y_value->IsUnitValue() ||
      !z_value->IsUnitValue()) {
    exception_state.ThrowTypeError("CSSMathValue are not supported yet");
    return nullptr;
  }

  if (x_value->GetType() != CSSStyleValue::StyleValueType::kNumberType ||
      y_value->GetType() != CSSStyleValue::StyleValueType::kNumberType ||
      z_value->GetType() != CSSStyleValue::StyleValueType::kNumberType) {
    exception_state.ThrowTypeError("CSSScale does not support non-number type");
    return nullptr;
  }

  return new CSSScale(x_value, y_value, z_value);
}

CSSScale* CSSScale::FromCSSValue(const CSSFunctionValue& value) {
  switch (value.FunctionType()) {
    case CSSValueScale:
      return FromScale(value);
    case CSSValueScaleX:
    case CSSValueScaleY:
    case CSSValueScaleZ:
      return FromScaleXYZ(value);
    case CSSValueScale3d:
      return FromScale3d(value);
    default:
      NOTREACHED();
      return nullptr;
  }
}

void CSSScale::setX(const CSSNumberish& x, ExceptionState& exception_state) {
  CSSNumericValue* value = CSSNumericValue::FromNumberish(x);

  if (!value->IsUnitValue()) {
    exception_state.ThrowTypeError("CSSMathValue are not supported yet");
    return;
  }

  if (value->GetType() != CSSStyleValue::StyleValueType::kNumberType) {
    exception_state.ThrowTypeError("Must specify an number unit");
    return;
  }

  x_ = value;
}
void CSSScale::setY(const CSSNumberish& y, ExceptionState& exception_state) {
  CSSNumericValue* value = CSSNumericValue::FromNumberish(y);

  if (!value->IsUnitValue()) {
    exception_state.ThrowTypeError("CSSMathValue are not supported yet");
    return;
  }

  if (value->GetType() != CSSStyleValue::StyleValueType::kNumberType) {
    exception_state.ThrowTypeError("Must specify an number unit");
    return;
  }

  y_ = value;
}
void CSSScale::setZ(const CSSNumberish& z, ExceptionState& exception_state) {
  CSSNumericValue* value = CSSNumericValue::FromNumberish(z);

  if (!value->IsUnitValue()) {
    exception_state.ThrowTypeError("CSSMathValue are not supported yet");
    return;
  }

  if (value->GetType() != CSSStyleValue::StyleValueType::kNumberType) {
    exception_state.ThrowTypeError("Must specify an number unit");
    return;
  }

  z_ = value;
}

const DOMMatrix* CSSScale::AsMatrix(ExceptionState& exception_state) const {
  CSSUnitValue* x = x_->to(CSSPrimitiveValue::UnitType::kNumber);
  CSSUnitValue* y = y_->to(CSSPrimitiveValue::UnitType::kNumber);
  CSSUnitValue* z = z_->to(CSSPrimitiveValue::UnitType::kNumber);

  if (!x || !y || !z) {
    exception_state.ThrowTypeError(
        "Cannot create matrix if valuse are not convert to CSSUnitValue");
    return nullptr;
  }

  DOMMatrix* result = DOMMatrix::Create();
  return result->scaleSelf(x->value(), y->value(), z->value());
}

const CSSFunctionValue* CSSScale::ToCSSValue() const {
  CSSFunctionValue* result =
      CSSFunctionValue::Create(is2D() ? CSSValueScale : CSSValueScale3d);
  result->Append(*x_->ToCSSValue());
  result->Append(*y_->ToCSSValue());
  if (!is2D()) {
    result->Append(*z_->ToCSSValue());
  }
  return result;
}

}  // namespace blink
