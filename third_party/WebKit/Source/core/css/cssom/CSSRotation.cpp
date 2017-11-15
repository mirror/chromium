// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/cssom/CSSRotation.h"

#include "bindings/core/v8/ExceptionState.h"
#include "core/css/CSSFunctionValue.h"
#include "core/css/CSSPrimitiveValue.h"
#include "core/css/cssom/CSSUnitValue.h"
#include "core/geometry/DOMMatrix.h"

namespace blink {

namespace {

bool IsNumberValue(const CSSValue& value) {
  return value.IsPrimitiveValue() && ToCSSPrimitiveValue(value).IsNumber();
}

CSSRotation* FromCSSRotate(const CSSFunctionValue& value) {
  DCHECK_EQ(value.length(), 1UL);
  const CSSPrimitiveValue& primitive_value = ToCSSPrimitiveValue(value.Item(0));
  if (primitive_value.IsCalculated() || !primitive_value.IsAngle())
    return nullptr;
  return CSSRotation::Create(CSSNumericValue::FromCSSValue(primitive_value));
}

CSSRotation* FromCSSRotate3d(const CSSFunctionValue& value) {
  DCHECK_EQ(value.length(), 4UL);
  DCHECK(IsNumberValue(value.Item(0)));
  DCHECK(IsNumberValue(value.Item(1)));
  DCHECK(IsNumberValue(value.Item(2)));
  const CSSPrimitiveValue& angle = ToCSSPrimitiveValue(value.Item(3));
  if (angle.IsCalculated() || !angle.IsAngle())
    return nullptr;

  CSSNumericValue* x =
      CSSNumericValue::FromCSSValue(ToCSSPrimitiveValue(value.Item(0)));
  CSSNumericValue* y =
      CSSNumericValue::FromCSSValue(ToCSSPrimitiveValue(value.Item(1)));
  CSSNumericValue* z =
      CSSNumericValue::FromCSSValue(ToCSSPrimitiveValue(value.Item(2)));

  return CSSRotation::Create(x, y, z, CSSNumericValue::FromCSSValue(angle));
}

CSSRotation* FromCSSRotateXYZ(const CSSFunctionValue& value) {
  DCHECK_EQ(value.length(), 1UL);
  const CSSPrimitiveValue& primitive_value = ToCSSPrimitiveValue(value.Item(0));
  if (primitive_value.IsCalculated())
    return nullptr;
  CSSNumericValue* angle = CSSNumericValue::FromCSSValue(primitive_value);
  switch (value.FunctionType()) {
    case CSSValueRotateX:
      return CSSRotation::Create(
          CSSUnitValue::Create(1, CSSPrimitiveValue::UnitType::kPixels),
          CSSUnitValue::Create(0, CSSPrimitiveValue::UnitType::kPixels),
          CSSUnitValue::Create(0, CSSPrimitiveValue::UnitType::kPixels), angle);
    case CSSValueRotateY:
      return CSSRotation::Create(
          CSSUnitValue::Create(0, CSSPrimitiveValue::UnitType::kPixels),
          CSSUnitValue::Create(1, CSSPrimitiveValue::UnitType::kPixels),
          CSSUnitValue::Create(0, CSSPrimitiveValue::UnitType::kPixels), angle);
    case CSSValueRotateZ:
      return CSSRotation::Create(
          CSSUnitValue::Create(0, CSSPrimitiveValue::UnitType::kPixels),
          CSSUnitValue::Create(0, CSSPrimitiveValue::UnitType::kPixels),
          CSSUnitValue::Create(1, CSSPrimitiveValue::UnitType::kPixels), angle);
    default:
      NOTREACHED();
      return nullptr;
  }
}

}  // namespace

CSSRotation* CSSRotation::Create(CSSNumericValue* angle,
                                 ExceptionState& exception_state) {
  if (angle->GetType() != CSSStyleValue::StyleValueType::kAngleType) {
    exception_state.ThrowTypeError("Must pass an angle to CSSRotation");
    return nullptr;
  }
  return new CSSRotation(
      CSSUnitValue::Create(0, CSSPrimitiveValue::UnitType::kPixels),
      CSSUnitValue::Create(0, CSSPrimitiveValue::UnitType::kPixels),
      CSSUnitValue::Create(1, CSSPrimitiveValue::UnitType::kPixels), angle,
      true /* is2D */);
}

CSSRotation* CSSRotation::Create(const CSSNumberish& x,
                                 const CSSNumberish& y,
                                 const CSSNumberish& z,
                                 CSSNumericValue* angle,
                                 ExceptionState& exception_state) {
  if (angle->GetType() != CSSStyleValue::StyleValueType::kAngleType) {
    exception_state.ThrowTypeError("Must pass an angle to CSSRotation");
    return nullptr;
  }
  return new CSSRotation(
      CSSNumericValue::FromNumberish(x), CSSNumericValue::FromNumberish(y),
      CSSNumericValue::FromNumberish(z), angle, false /* is2D */);
}

CSSRotation* CSSRotation::Create(CSSNumericValue* angle) {
  DCHECK_EQ(angle->GetType(), CSSStyleValue::StyleValueType::kAngleType);
  return new CSSRotation(
      CSSUnitValue::Create(0, CSSPrimitiveValue::UnitType::kPixels),
      CSSUnitValue::Create(0, CSSPrimitiveValue::UnitType::kPixels),
      CSSUnitValue::Create(1, CSSPrimitiveValue::UnitType::kPixels), angle,
      true /* is2D */);
}

CSSRotation* CSSRotation::Create(CSSNumericValue* x,
                                 CSSNumericValue* y,
                                 CSSNumericValue* z,
                                 CSSNumericValue* angle) {
  DCHECK_EQ(angle->GetType(), CSSStyleValue::StyleValueType::kAngleType);
  return new CSSRotation(x, y, z, angle, false /* is2D */);
}

CSSRotation* CSSRotation::FromCSSValue(const CSSFunctionValue& value) {
  switch (value.FunctionType()) {
    case CSSValueRotate:
      return FromCSSRotate(value);
    case CSSValueRotate3d:
      return FromCSSRotate3d(value);
    case CSSValueRotateX:
    case CSSValueRotateY:
    case CSSValueRotateZ:
      return FromCSSRotateXYZ(value);
    default:
      NOTREACHED();
      return nullptr;
  }
}

void CSSRotation::setAngle(CSSNumericValue* angle,
                           ExceptionState& exception_state) {
  if (angle->GetType() != CSSStyleValue::StyleValueType::kAngleType) {
    exception_state.ThrowTypeError("Must pass an angle to CSSRotation");
    return;
  }
  if (angle->IsCalculated()) {
    exception_state.ThrowTypeError("Calculated angles are not supported yet");
    return;
  }
  angle_ = angle;
}

const DOMMatrix* CSSRotation::AsMatrix(ExceptionState& exception_state) const {
  CSSUnitValue* x = x_->to(CSSPrimitiveValue::UnitType::kPixels);
  CSSUnitValue* y = y_->to(CSSPrimitiveValue::UnitType::kPixels);
  CSSUnitValue* z = z_->to(CSSPrimitiveValue::UnitType::kPixels);
  if (!x || !y || !z) {
    exception_state.ThrowTypeError(
        "Cannot create matrix if units are not compatible with px");
    return nullptr;
  }

  DOMMatrix* matrix = DOMMatrix::Create();
  CSSUnitValue* angle = angle_->to(CSSPrimitiveValue::UnitType::kDegrees);
  if (is2D()) {
    matrix->rotateAxisAngleSelf(0, 0, 1, angle->value());
  } else {
    matrix->rotateAxisAngleSelf(x->value(), y->value(), z->value(),
                                angle->value());
  }
  return matrix;
}

const CSSFunctionValue* CSSRotation::ToCSSValue() const {
  // TODO(meade): Handle calc angles.
  CSSUnitValue* x = x_->to(CSSPrimitiveValue::UnitType::kPixels);
  CSSUnitValue* y = y_->to(CSSPrimitiveValue::UnitType::kPixels);
  CSSUnitValue* z = z_->to(CSSPrimitiveValue::UnitType::kPixels);
  if (!x || !y || !z) {
    return nullptr;
  }
  CSSUnitValue* angle = ToCSSUnitValue(angle_);
  CSSFunctionValue* result =
      CSSFunctionValue::Create(is2D() ? CSSValueRotate : CSSValueRotate3d);
  if (!is2D()) {
    result->Append(*CSSPrimitiveValue::Create(
        x->value(), CSSPrimitiveValue::UnitType::kNumber));
    result->Append(*CSSPrimitiveValue::Create(
        y->value(), CSSPrimitiveValue::UnitType::kNumber));
    result->Append(*CSSPrimitiveValue::Create(
        z->value(), CSSPrimitiveValue::UnitType::kNumber));
  }
  result->Append(
      *CSSPrimitiveValue::Create(angle->value(), angle->GetInternalUnit()));
  return result;
}

}  // namespace blink
