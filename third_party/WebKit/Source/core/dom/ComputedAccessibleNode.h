// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ComputedAccessibleNode_h
#define ComputedAccessibleNode_h

#include <string>

#include "platform/bindings/ScriptWrappable.h"

namespace blink {

class ComputedAccessibleNode {
  DEFINE_WRAPPERTYPEINFO();

 public:
  explicit ComputedAccessibleNode(Element*);
  virtual ~ComputedAccessibleNode();

  std::string name() const;
  void setName(const std::string&);

  std::string role() const;
  void setRole(const std::string&);

 private:
  std::string name_;
  std::string role_;
};

}  // namespace blink

#endif  // ComputedAccessibleNode_h
