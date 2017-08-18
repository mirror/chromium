// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/character_data.h"

#include "third_party/WebKit/Source/core/dom/CharacterData.h"

namespace webagents {

CharacterData::CharacterData(blink::CharacterData* cdata) : Node(cdata) {}

CharacterData* CharacterData::Create(blink::CharacterData* cdata) {
  return cdata ? new CharacterData(cdata) : nullptr;
}

blink::CharacterData* CharacterData::GetCharacterData() const {
  return blink::ToCharacterData(GetNode());
}

unsigned CharacterData::length() const {
  return GetCharacterData()->length();
}

}  // namespace webagents
