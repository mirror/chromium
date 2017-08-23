// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_CHARACTER_DATA_H_
#define WEBAGENTS_CHARACTER_DATA_H_

#include "third_party/WebKit/webagents/node.h"

namespace blink {
class CharacterData;
}

namespace webagents {

// https://dom.spec.whatwg.org/#interface-characterdata
class CharacterData : public Node {
 public:
  CharacterData(Node&);
  virtual ~CharacterData() = default;

  // readonly attribute unsigned long length;
  unsigned length() const;

 protected:
  explicit CharacterData(blink::CharacterData*);
  blink::CharacterData* GetCharacterData() const;
};

}  // namespace webagents

#endif  // WEBAGENTS_CHARACTER_DATA_H_
