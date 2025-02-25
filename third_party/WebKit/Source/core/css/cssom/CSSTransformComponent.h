// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CSSTransformComponent_h
#define CSSTransformComponent_h

#include "base/macros.h"
#include "core/CoreExport.h"
#include "core/css/CSSFunctionValue.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class DOMMatrix;
class ExceptionState;

// CSSTransformComponent is the base class used for the representations of
// the individual CSS transforms. They are combined in a CSSTransformValue
// before they can be used as a value for properties like "transform".
// See CSSTransformComponent.idl for more information about this class.
class CORE_EXPORT CSSTransformComponent : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  enum TransformComponentType {
    kMatrixType,
    kPerspectiveType,
    kRotationType,
    kScaleType,
    kSkewType,
    kSkewXType,
    kTranslationType,
  };

  virtual ~CSSTransformComponent() = default;

  // Blink-internal ways of creating CSSTransformComponents.
  static CSSTransformComponent* FromCSSValue(const CSSValue&);

  // Getters and setters for attributes defined in the IDL.
  bool is2D() const { return is2D_; }
  virtual void setIs2D(bool is2D) { is2D_ = is2D; }
  String toString() const;

  // Internal methods.
  virtual TransformComponentType GetType() const = 0;
  virtual const CSSFunctionValue* ToCSSValue() const = 0;
  virtual const DOMMatrix* AsMatrix(ExceptionState&) const = 0;

 protected:
  CSSTransformComponent(bool is2D) : is2D_(is2D) {}

 private:
  bool is2D_;
  DISALLOW_COPY_AND_ASSIGN(CSSTransformComponent);
};

}  // namespace blink

#endif
