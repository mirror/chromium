// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/html_form_controls_collection.h"

#include "core/html/HTMLFormControlsCollection.h"

namespace webagents {

HTMLFormControlsCollection::HTMLFormControlsCollection(
    blink::HTMLFormControlsCollection& collection)
    : HTMLCollection(collection) {}

}  // namespace webagents
