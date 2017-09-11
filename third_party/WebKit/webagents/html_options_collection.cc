// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/html_options_collection.h"

#include "core/html/HTMLOptionsCollection.h"

namespace webagents {

HTMLOptionsCollection::HTMLOptionsCollection(
    blink::HTMLOptionsCollection& collection)
    : HTMLCollection(collection) {}

}  // namespace webagents
