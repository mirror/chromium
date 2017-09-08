// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/html_collection.h"

#include "core/html/HTMLCollection.h"
#include "third_party/WebKit/webagents/element.h"

namespace webagents {

Element HTMLCollectionIterator::operator*() const {
  return Element(*collection_.private_->item(index_));
}

bool HTMLCollectionIterator::operator!=(
    const HTMLCollectionIterator& rval) const {
  return collection_ != rval.collection_ || index_ != rval.index_;
}

HTMLCollection::~HTMLCollection() {
  private_.Reset();
}

HTMLCollection::HTMLCollection(blink::HTMLCollection& collection)
    : private_(&collection) {}

HTMLCollection::HTMLCollection(const HTMLCollection& collection) {
  Assign(collection);
}

void HTMLCollection::Assign(const HTMLCollection& collection) {
  private_ = collection.private_;
}

bool HTMLCollection::Equals(const HTMLCollection& collection) const {
  return private_.Get() == collection.private_.Get();
}

int HTMLCollection::length() const {
  return private_->length();
}

}  // namespace webagents
