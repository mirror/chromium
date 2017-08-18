// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/text.h"

#include "third_party/WebKit/Source/core/dom/Text.h"

namespace webagents {

Text::Text(blink::Text* text) : CharacterData(text) {}

Text* Text::Create(blink::Text* text) {
  return text ? new Text(text) : nullptr;
}

blink::Text* Text::GetText() const {
  return blink::ToText(GetNode());
}

}  // namespace webagents
