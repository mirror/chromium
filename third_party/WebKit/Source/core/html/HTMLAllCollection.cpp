/*
 * Copyright (C) 2009, 2011, 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "core/html/HTMLAllCollection.h"
#include "bindings/core/v8/html_collection_or_element.h"
#include "core/dom/Element.h"

namespace blink {

HTMLAllCollection* HTMLAllCollection::Create(ContainerNode& node,
                                             CollectionType type) {
  DCHECK_EQ(type, kDocAll);
  return new HTMLAllCollection(node);
}

HTMLAllCollection::HTMLAllCollection(ContainerNode& node)
    : HTMLCollection(node, kDocAll, kDoesNotOverrideItemAfter) {}

HTMLAllCollection::~HTMLAllCollection() = default;

Element* HTMLAllCollection::AnonymousIndexedGetter(unsigned index) {
  return HTMLCollection::item(index);
}

void HTMLAllCollection::NamedGetter(const AtomicString& name,
                                    HTMLCollectionOrElement& return_value) {
  HTMLCollection* items = GetDocument().DocumentAllNamedItems(name);

  if (!items->length())
    return;

  if (items->length() == 1) {
    return_value.SetElement(items->item(0));
    return;
  }

  return_value.SetHTMLCollection(items);
}

void HTMLAllCollection::item(HTMLCollectionOrElement& return_value) {
  DCHECK(return_value.IsNull());
}

void HTMLAllCollection::item(const AtomicString& name_or_index,
                             HTMLCollectionOrElement& return_value) {
  bool ok = true;
  int64_t index = name_or_index.GetString().ToInt64Strict(&ok);
  if (ok && index >= 0 && index < 4294967295) {
    if (String::Number(index) == name_or_index) {
      Element* element = HTMLCollection::item(static_cast<unsigned>(index));
      return_value.SetElement(element);
      return;
    }
  }

  NamedGetter(name_or_index, return_value);
}

}  // namespace blink
