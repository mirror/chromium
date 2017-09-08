// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_HTML_COLLECTION_H_
#define WEBAGENTS_HTML_COLLECTION_H_

#include "third_party/WebKit/public/platform/WebPrivatePtr.h"
#include "third_party/WebKit/webagents/html_collection.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class HTMLCollection;
}

namespace webagents {

class HTMLCollection;
class Element;

class WEBAGENTS_EXPORT HTMLCollectionIterator {
 public:
  explicit HTMLCollectionIterator(const HTMLCollection& collection, int index)
      : collection_(collection), index_(index) {}

  Element operator*() const;
  bool operator!=(const HTMLCollectionIterator& rval) const;
  HTMLCollectionIterator& operator++() {
    ++index_;
    return *this;
  }

 private:
  const HTMLCollection& collection_;
  int index_;
};

// https://dom.spec.whatwg.org/#interface-htmlcollection
class WEBAGENTS_EXPORT HTMLCollection {
 public:
  virtual ~HTMLCollection();

  HTMLCollection(const HTMLCollection&);
  void Assign(const HTMLCollection& collection);
  bool Equals(const HTMLCollection& collection) const;
  friend bool operator==(const HTMLCollection& lhs, const HTMLCollection& rhs) {
    return lhs.Equals(rhs);
  }
  friend bool operator!=(const HTMLCollection& lhs, const HTMLCollection& rhs) {
    return !lhs.Equals(rhs);
  }

  using Iterator = HTMLCollectionIterator;
  Iterator begin() const { return Iterator(*this, 0); }
  Iterator end() const { return Iterator(*this, length()); }

  // readonly attribute unsigned long length;
  int length() const;

#if INSIDE_BLINK
 protected:
  template <class T>
  const T& ConstUnwrap() const {
    return static_cast<const T&>(*private_);
  }

  friend class Element;
  explicit HTMLCollection(blink::HTMLCollection&);
#endif

 private:
  friend class HTMLCollectionIterator;
  blink::WebPrivatePtr<blink::HTMLCollection> private_;
};

}  // namespace webagents

#endif  // WEBAGENTS_HTML_COLLECTION_H_
