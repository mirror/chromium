// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_TEXT_H_
#define WEBAGENTS_TEXT_H_

#include "third_party/WebKit/webagents/character_data.h"

namespace blink {
class Text;
}

namespace webagents {

// https://dom.spec.whatwg.org/#interface-text
class Text : public CharacterData {
 public:
  Text(Node&);
  Text(const Node&);
  virtual ~Text() = default;

 protected:
  explicit Text(blink::Text*);
  blink::Text* GetText() const;
};

}  // namespace webagents

#endif  // WEBAGENTS_TEXT_H_
