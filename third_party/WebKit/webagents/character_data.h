// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_CHARACTER_DATA_H_
#define WEBAGENTS_CHARACTER_DATA_H_

#include "third_party/WebKit/webagents/node.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class CharacterData;
}

namespace webagents {

// https://dom.spec.whatwg.org/#interface-characterdata
class WEBAGENTS_EXPORT CharacterData : public Node {
 public:
  // readonly attribute unsigned long length;
  unsigned length() const;

#if INSIDE_BLINK
 protected:
  explicit CharacterData(blink::CharacterData&);
#endif
};

}  // namespace webagents

#endif  // WEBAGENTS_CHARACTER_DATA_H_
