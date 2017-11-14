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
  CSSNumericValue* x =
      CSSNumericValue::FromCSSValue(ToCSSPrimitiveValue(value.Item(0)));
  if (value.length() == 1U)
    return CSSScale::Create(x, x);

  CSSNumericValue* y =
      CSSNumericValue::FromCSSValue(ToCSSPrimitiveValue(value.Item(1)));
  return CSSScale::Create(x, y);
}

CSSScale* FromScaleXYZ(const CSSFunctionValue& value) {
  DCHECK_EQ(value.length(), 1U);
  CSSNumericValue* numeric_value =
      CSSNumericValue::FromCSSValue(ToCSSPrimitiveValue(value.Item(0)));
  CSSUnitValue* default_value =
      CSSUnitValue::Create(1, CSSPrimitiveValue::UnitType::kPixels);
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
  CSSNumericValue* x =
      CSSNumericValue::FromCSSValue(ToCSSPrimitiveValue(value.Item(0)));
  CSSNumericValue* y =
      CSSNumericValue::FromCSSValue(ToCSSPrimitiveValue(value.Item(1)));
  CSSNumericValue* z =
      CSSNumericValue::FromCSSValue(ToCSSPrimitiveValue(value.Item(2)));
  return CSSScale::Create(x, y, z);
}

}  // namespace

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

const DOMMatrix* CSSScale::AsMatrix(ExceptionState& exception_state) const {
  CSSUnitValue* x = x_->to(CSSPrimitiveValue::UnitType::kPixels);
  CSSUnitValue* y = y_->to(CSSPrimitiveValue::UnitType::kPixels);
  CSSUnitValue* z = z_->to(CSSPrimitiveValue::UnitType::kPixels);

  if (!x || !y || !z) {
    exception_state.ThrowTypeError(
        "Cannot create matrix if units are not compatible with px");
    return nullptr;
  }

  DOMMatrix* result = DOMMatrix::Create();
  return result->scaleSelf(x->value(), y->value(), z->value());
}

const CSSFunctionValue* CSSScale::ToCSSValue() const {
  CSSFunctionValue* result =
      CSSFunctionValue::Create(is2D() ? CSSValueScale : CSSValueScale3d);
  CSSUnitValue* x = ToCSSUnitValue(x_);
  CSSUnitValue* y = ToCSSUnitValue(y_);
  result->Append(*CSSPrimitiveValue::Create(x->value(), x->GetInternalUnit()));
  result->Append(*CSSPrimitiveValue::Create(y->value(), y->GetInternalUnit()));
  if (!is2D()) {
    CSSUnitValue* z = ToCSSUnitValue(z_);
    result->Append(
        *CSSPrimitiveValue::Create(z->value(), z->GetInternalUnit()));
  }
  return result;
}

}  // namespace blink
