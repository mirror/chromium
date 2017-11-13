// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CSSScale_h
#define CSSScale_h

#include "core/css/cssom/CSSTransformComponent.h"
#include "core/geometry/DOMMatrix.h"

namespace blink {

class DOMMatrix;

// Represents a scale value in a CSSTransformValue used for properties like
// "transform".
// See CSSScale.idl for more information about this class.
class CORE_EXPORT CSSScale final : public CSSTransformComponent {
  WTF_MAKE_NONCOPYABLE(CSSScale);
  DEFINE_WRAPPERTYPEINFO();

 public:
  // Constructors defined in the IDL.
  static CSSScale* Create(const CSSNumberish& x, const CSSNumberish& y) {
    return new CSSScale(x, y);
  }
  static CSSScale* Create(const CSSNumberish& x,
                          const CSSNumberish& y,
                          const CSSNumberish& z) {
    return new CSSScale(x, y, z);
  }

  // Blink-internal ways of creating CSSScales.
  static CSSScale* FromCSSValue(const CSSFunctionValue&);

  // Getters and setters for attributes defined in the IDL.
  void x(CSSNumberish& x) { x.SetCSSNumericValue(x_); }
  void y(CSSNumberish& y) { y.SetCSSNumericValue(y_); }
  void z(CSSNumberish& z) { z.SetCSSNumericValue(z_); }
  void setX(const CSSNumberish& x) { x_ = CSSNumericValue::FromNumberish(x); }
  void setY(const CSSNumberish& y) { y_ = CSSNumericValue::FromNumberish(y); }
  void setZ(const CSSNumberish& z) { z_ = CSSNumericValue::FromNumberish(z); }

  // Internal methods - from CSSTransformComponent.
  TransformComponentType GetType() const final { return kScaleType; }
  const DOMMatrix* AsMatrix(ExceptionState&) const final;
  const CSSFunctionValue* ToCSSValue() const final;

 private:
  CSSScale(CSSNumericValue* x, CSSNumericValue* y)
      : CSSTransformComponent(true /* is2D */),
        x_(x),
        y_(y),
        z_(CSSUnitValue::Create(1, CSSPrimitiveValue::UnitType::kPixels)) {}
  CSSScale(CSSNumericValue* x, CSSNumericValue* y, CSSNumericValue* z)
      : CSSTransformComponent(false /* is2D */), x_(x), y_(y), z_(z) {}

  Member<CSSNumericValue> x_;
  Member<CSSNumericValue> y_;
  Member<CSSNumericValue> z_;
};

}  // namespace blink

#endif
