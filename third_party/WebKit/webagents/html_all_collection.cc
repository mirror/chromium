// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/html_all_collection.h"

#include "core/html/HTMLAllCollection.h"

namespace webagents {

HTMLAllCollection::HTMLAllCollection(blink::HTMLAllCollection& collection)
    : HTMLCollection(collection) {}

}  // namespace webagents
