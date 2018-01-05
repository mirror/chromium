// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebIDBValue_h
#define WebIDBValue_h

#include <string>
#include <vector>

#include "public/platform/WebBlobInfo.h"
#include "public/platform/WebCommon.h"
#include "public/platform/WebVector.h"
#include "public/platform/modules/indexeddb/WebIDBKey.h"

namespace blink {

class IDBValue;
class WebData;
class WebIDBKeyPath;

// Handle to an IndexedDB Object Store value retrieved from the backing store.
class WebIDBValue {
 public:
  BLINK_EXPORT WebIDBValue(const WebData&, const WebVector<WebBlobInfo>&);

  BLINK_EXPORT WebIDBValue(WebIDBValue&& other) noexcept;
  BLINK_EXPORT WebIDBValue& operator=(WebIDBValue&&) noexcept;

  BLINK_EXPORT ~WebIDBValue();

  // Used by object stores that store primary keys separately from wire data.
  BLINK_EXPORT void SetInjectedPrimaryKey(
      WebIDBKey primary_key,
      const WebIDBKeyPath& primary_key_path);

  // Appends the Blob UUIDs associated with this value to the receiver vector.
  //
  // std::vector<std::string> is used instead of WebVector<WebString> to avoid
  // extra allocations and copies at this method's call site.
  BLINK_EXPORT void AppendBlobUuidsTo(std::vector<std::string>& receiver) const;

#if INSIDE_BLINK
  std::unique_ptr<IDBValue> ReleaseIdbValue() noexcept {
    // The code would be less verbose (but slightly more magic) if this method
    // were an implicit cast (operator std::unique_ptr<IDBKey> () && noexcept).
    // While the alternative is tempting and has precedent (implicit cast
    // operators are used by some blink::Web* types), it confuses some of the
    // supported compilers.
    return std::move(private_);
  }
#endif  // INSIDE_BLINK

 private:
  // WebIDBValue has to be move-only, because std::unique_ptr is move-only.
  // Making the restriction explicit results in slightly better compilation
  // error messages in code that attempts copying.
  WebIDBValue(const WebIDBValue&) = delete;
  WebIDBValue& operator=(const WebIDBValue&) = delete;

  std::unique_ptr<IDBValue> private_;
};

}  // namespace blink

#endif  // WebIDBValue_h
