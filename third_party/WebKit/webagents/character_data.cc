// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/character_data.h"

#include "core/dom/CharacterData.h"

namespace webagents {

CharacterData::CharacterData(blink::CharacterData& cdata) : Node(cdata) {}

unsigned CharacterData::length() const {
  return ConstUnwrap<blink::CharacterData>().length();
}

}  // namespace webagents
