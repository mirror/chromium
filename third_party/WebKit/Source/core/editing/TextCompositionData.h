// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TextCompositionData_h
#define TextCompositionData_h

#include "core/CoreExport.h"
#include "platform/wtf/Vector.h"

namespace blink {

class CompositionUnderline;
struct WebTextCompositionData;

class CORE_EXPORT TextCompositionData {
 public:
  TextCompositionData();
  TextCompositionData(const Vector<CompositionUnderline>&);
  TextCompositionData(const WebTextCompositionData&);

  ~TextCompositionData();

  // TODO: are these needed?
  /*
  bool operator==(const TextCompositionData& rhs) const;
  bool operator!=(const TextCompositionData& rhs) const;
  */

  const Vector<CompositionUnderline>& CompositionUnderlines() const {
    return composition_underlines_;
  }

 private:
  Vector<CompositionUnderline> composition_underlines_;
};

}  // namespace blink

#endif  // TextCompositionData_h
