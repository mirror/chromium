// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/cssom/CSSMatrixComponent.h"

#include "core/css/CSSPrimitiveValue.h"
#include "platform/wtf/MathExtras.h"

namespace blink {

CSSMatrixComponent* CSSMatrixComponent::Create(DOMMatrixReadOnly* matrix) {
  return new CSSMatrixComponent(matrix);
}

CSSFunctionValue* CSSMatrixComponent::ToCSSValue() const {
  CSSFunctionValue* result =
      CSSFunctionValue::Create(is2d_ ? CSSValueMatrix : CSSValueMatrix3d);

  if (is2d_) {
    double values[6] = {matrix_->a(), matrix_->b(), matrix_->c(),
                        matrix_->d(), matrix_->e(), matrix_->f()};
    for (double value : values) {
      result->Append(*CSSPrimitiveValue::Create(
          value, CSSPrimitiveValue::UnitType::kNumber));
    }
  } else {
    double values[16] = {
        matrix_->m11(), matrix_->m12(), matrix_->m13(), matrix_->m14(),
        matrix_->m21(), matrix_->m22(), matrix_->m23(), matrix_->m24(),
        matrix_->m31(), matrix_->m32(), matrix_->m33(), matrix_->m34(),
        matrix_->m41(), matrix_->m42(), matrix_->m43(), matrix_->m44()};
    for (double value : values) {
      result->Append(*CSSPrimitiveValue::Create(
          value, CSSPrimitiveValue::UnitType::kNumber));
    }
  }

  return result;
}

CSSMatrixComponent::CSSMatrixComponent(DOMMatrixReadOnly* matrix)
    : CSSTransformComponent() {
  matrix_ = DOMMatrix::Create(matrix);
  is2d_ = matrix->is2D();
}

}  // namespace blink
