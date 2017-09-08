// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/html_head_element.h"

#include "core/html/HTMLHeadElement.h"

namespace webagents {

HTMLHeadElement::HTMLHeadElement(blink::HTMLHeadElement& element)
    : HTMLElement(element) {}

}  // namespace webagents
