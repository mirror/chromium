// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEB_AGENT_API_ELEMENT_H_
#define WEB_AGENT_API_ELEMENT_H_

#include "core/web_agent/api/node.h"

namespace blink {
class Element;
}

namespace web {

class Element : public Node {
 public:
  virtual ~Element() = default;
  static Element* Create(blink::Element*);

 protected:
  explicit Element(blink::Element*);
  blink::Element* GetElement() const;
};

}  // namespace web

#endif  // WEB_AGENT_API_ELEMENT_H_
