// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_ELEMENT_H_
#define WEBAGENTS_ELEMENT_H_

#include "third_party/WebKit/webagents/node.h"

namespace blink {
class Element;
}

namespace webagents {

class Element : public Node {
 public:
  virtual ~Element() = default;
  static Element* Create(blink::Element*);

 protected:
  explicit Element(blink::Element*);
  blink::Element* GetElement() const;
};

}  // namespace webagents

#endif  // WEBAGENTS_ELEMENT_H_
