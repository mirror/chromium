// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef StylePropertyMap_h
#define StylePropertyMap_h

#include "bindings/core/v8/css_style_value_or_string.h"
#include "bindings/core/v8/v8_update_function.h"
#include "core/css/cssom/StylePropertyMapReadonly.h"

namespace blink {

class ExceptionState;

class CORE_EXPORT StylePropertyMap : public StylePropertyMapReadonly {
  DEFINE_WRAPPERTYPEINFO();
  WTF_MAKE_NONCOPYABLE(StylePropertyMap);

 public:
  void set(const String& property_name,
           HeapVector<CSSStyleValueOrString>& values,
           ExceptionState&);
  void append(const String& property_name,
              HeapVector<CSSStyleValueOrString>& values,
              ExceptionState&);
  void remove(const String& property_name, ExceptionState&);
  void update(const String&, const V8UpdateFunction*) {}

  virtual void remove(CSSPropertyID, ExceptionState&) = 0;

 protected:
  virtual void SetProperty(CSSPropertyID, const CSSValue*) = 0;

  StylePropertyMap() {}
};

}  // namespace blink

#endif
