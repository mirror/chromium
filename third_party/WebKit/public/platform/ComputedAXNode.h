// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ComputedAXNode_h
#define ComputedAXNode_h

#include <string>
#include <vector>

namespace blink {

class ComputedAXNode {
 public:
  ~ComputedAXNode() = default;

  virtual bool GetScrollable() = 0;
  virtual float GetFontSize() = 0;
  virtual int GetColorValue() = 0;
  virtual const std::string& GetName() = 0;
  virtual const std::string& GetRole() = 0;
  virtual const std::vector<int32_t>& GetLabelledByIds() = 0;
};

}  // namespace blink

#endif  // ComputedAXNode_h
