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

class WebIDBKey {
 public:
  BLINK_EXPORT static WebIDBKey CreateArray(WebVector<WebIDBKey>);
  BLINK_EXPORT static WebIDBKey CreateBinary(const WebData&);
  BLINK_EXPORT static WebIDBKey CreateString(const WebString&);
  BLINK_EXPORT static WebIDBKey CreateDate(double);
  BLINK_EXPORT static WebIDBKey CreateNumber(double);
  BLINK_EXPORT static WebIDBKey CreateInvalid();
  BLINK_EXPORT static WebIDBKey CreateNull();

  WebIDBKey(WebIDBKey&&);
  WebIDBKey& operator=(WebIDBKey&&);

  ~WebIDBKey();

  BLINK_EXPORT WebIDBKeyType KeyType() const;
  BLINK_EXPORT bool IsValid() const;
  // Only valid for ArrayType.
  BLINK_EXPORT WebVector<WebIDBKey> TakeArray() &&;
  // Only valid for BinaryType.
  BLINK_EXPORT WebData TakeBinary() &&;
  // Only valid for StringType.
  BLINK_EXPORT WebString TakeString() &&;
  // Only valid for DateType.
  BLINK_EXPORT double TakeDate() &&;
  // Only valid for NumberType.
  BLINK_EXPORT double TakeNumber() &&;

#if INSIDE_BLINK
  WebIDBKey(std::unique_ptr<IDBKey> value) : private_(std::move(value)) {}
  WebIDBKey& operator=(std::unique_ptr<IDBKey> value) {
    private_ = std::move(value);
    return *this;
  }
  operator IDBKey*() const { return private_.get(); }

  // TODO(pwnall): This requires approval from C++ OWNERS.
  operator std::unique_ptr<IDBKey>() && { return std::move(private_); }
#endif

 private:
  friend class WebVector<WebIDBKey>;
  WebIDBKey();

  std::unique_ptr<IDBKey> private_;
};

}  // namespace blink

#endif  // WebIDBKey_h
