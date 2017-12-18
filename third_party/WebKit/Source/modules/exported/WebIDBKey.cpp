/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "public/platform/modules/indexeddb/WebIDBKey.h"

#include "modules/indexeddb/IDBKey.h"

namespace blink {

size_t WebIDBKeyArrayView::size() const {
  return private_->Array().size();
}

WebIDBKeyView WebIDBKeyArrayView::operator[](size_t index) const {
  return WebIDbKeyView(private_->Array()[index]);
}

WebIDBKeyType WebIDBKeyView::KeyType() const {
  if (!private_.get())
    return kWebIDBKeyTypeNull;
  return static_cast<WebIDBKeyType>(private_->GetType());
}

bool WebIDBKeyView::IsValid() const {
  if (!private_)
    return false;
  return private_->IsValid();
}

WebData WebIDBKeyView::Binary() {
  return private_->Binary();
}

WebString WebIDBKeyView::GetString() && {
  return private_->GetString();
}

double WebIDBKeyView::Date() {
  return private_->Date();
}

double WebIDBKeyView::Number() {
  return private_->Number();
}

WebIDBKey WebIDBKey::CreateArray(WebVector<WebIDBKey> array) {
  IDBKey::KeyArray keys;
  keys.ReserveCapacity(array.size());
  for (WebIDBKey& key : array) {
    DCHECK(key.KeyType() != kWebIDBKeyTypeNull);
    keys.emplace_back(static_cast<std::unique_ptr<IDBKey>>(std::move(key)));
  }
  return IDBKey::CreateArray(std::move(keys));
}

WebIDBKey WebIDBKey::CreateBinary(const WebData& binary) {
  return IDBKey::CreateBinary(binary);
}

WebIDBKey WebIDBKey::CreateString(const WebString& string) {
  return IDBKey::CreateString(string);
}

WebIDBKey WebIDBKey::CreateDate(double date) {
  return IDBKey::CreateDate(date);
}

WebIDBKey WebIDBKey::CreateNumber(double number) {
  return IDBKey::CreateNumber(number);
}

WebIDBKey WebIDBKey::CreateInvalid() {
  return IDBKey::CreateInvalid();
}

WebIDBKey WebIDBKey::CreateNull() {
  return WebIDBKey();
}

WebIDBKey::WebIDBKey(WebIDBKey&&) = default;
WebIDBKey& WebIDBKey::operator=(WebIDBKey&&) = default;

WebIDBKey::~WebIDBKey() = default;

}  // namespace blink
