/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebIDBKey_h
#define WebIDBKey_h

#include <memory>

#include "public/platform/WebCommon.h"
#include "public/platform/WebData.h"
#include "public/platform/WebPrivatePtr.h"
#include "public/platform/WebString.h"
#include "public/platform/WebVector.h"
#include "public/platform/modules/indexeddb/WebIDBTypes.h"

namespace blink {

class IDBKey;
class WebIDBKeyView;

// Minimal interface for iterating over an IndexedDB array key.
//
// See WebIDBKeyView for the rationale behind this class' existence.
class WebIDBKeyArrayView {
 public:
  BLINK_EXPORT size_t size() const;

  BLINK_EXPORT WebIDBKeyView operator[](size_t index) const;

 private:
  // Only WebIDBKeyView can vend WebIDBArrayKeyView instances.
  friend class WebIDBKeyView;
  explicit WebIDBKeyArrayView(const IDBKey* idb_key) : private_(idb_key) {}

  const IDBKey* const private_;
};

// Non-owning reference to an IndexedDB key.
//
// The Blink object wrapped by WebIDBKey is immutable, so WebIDBKeyView
// instances are implicitly const references.
//
// Having both WebIDBKeyView and WebIDBView is extra complexity, and we pay this
// price to avoid unnecessary memory copying. Specifically, WebIDBKeyView is
// used to issue requests to the IndexedDB backing store.
//
// For example, IDBCursor.update() must send the cursor's primary key to the
// backing store. IDBCursor cannot give up the ownership of its primary key,
// because it might need to satisfy further update() or delete() calls.
class WebIDBKeyView {
 public:
  BLINK_EXPORT WebIDBKeyType KeyType() const;

  BLINK_EXPORT bool IsValid() const;

  // Only valid for ArrayType.
  //
  // The caller is responsible for ensuring that the WebIDBKeyView is valid for
  // the lifetime of the returned WeIDBKeyArrayView.
  BLINK_EXPORT const WebIDBKeyArrayView ArrayView() const {
    return WebIDBKeyArrayView(private_);
  }

  // Only valid for BinaryType.
  BLINK_EXPORT WebData Binary() const;

  // Only valid for StringType.
  BLINK_EXPORT WebString GetString() const;

  // Only valid for DateType.
  BLINK_EXPORT double Date() const;

  // Only valid for NumberType.
  BLINK_EXPORT double Number() const;

 private:
  // Only WebIDBKey may vend WebIDBKeyView instances.
  friend class WebIDBKey;
  explicit WebIDBKeyView(const IDBKey* idb_key) : private_(idb_key) {}

  const IDBKey* const private_;
};

// Move-only handler that owns an IndexedDB key.
//
// The Blink object wrapped by WebIDBKey is immutable.
//
// Having both WebIDBKeyView and WebIDBView is extra complexity, and we pay this
// price to avoid unnecessary memory copying. Specifically, WebIDBKey is used to
// receive data from the IndexedDB backing store. Once constructed, a WebIDBKey
// is moved through the layer cake until the underlying Blink object ends up at
// its final destination.
class WebIDBKey {
 public:
  BLINK_EXPORT static WebIDBKey CreateArray(WebVector<WebIDBKey>);
  BLINK_EXPORT static WebIDBKey CreateBinary(const WebData&);
  BLINK_EXPORT static WebIDBKey CreateString(const WebString&);
  BLINK_EXPORT static WebIDBKey CreateDate(double);
  BLINK_EXPORT static WebIDBKey CreateNumber(double);
  BLINK_EXPORT static WebIDBKey CreateInvalid();
  BLINK_EXPORT static WebIDBKey CreateNull();

  BLINK_EXPORT WebIDBKey(WebIDBKey&&);
  BLINK_EXPORT WebIDBKey& operator=(WebIDBKey&&);

  BLINK_EXPORT ~WebIDBKey();

  BLINK_EXPORT WebIDBKeyView View() { return WebIDBKeyView(private_.get()); }

#if INSIDE_BLINK
  WebIDBKey(std::unique_ptr<IDBKey> idb_key) : private_(std::move(idb_key)) {}
  WebIDBKey& operator=(std::unique_ptr<IDBKey> idb_key) {
    private_ = std::move(idb_key);
    return *this;
  }
  operator IDBKey*() const { return private_.get(); }

  // TODO(pwnall): Ref-qualifier on member function requires C++ style approval.
  operator std::unique_ptr<IDBKey>() && { return std::move(private_); }
#endif

 private:
  // The default constructor is only defined for WebVector's use.
  friend class WebVector<WebIDBKey>;
  WebIDBKey();

  std::unique_ptr<IDBKey> private_;
};

}  // namespace blink

#endif  // WebIDBKey_h
