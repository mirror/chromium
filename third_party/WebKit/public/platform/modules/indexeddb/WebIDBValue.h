// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebIDBValue_h
#define WebIDBValue_h

#include "public/platform/WebBlobInfo.h"
#include "public/platform/WebData.h"
#include "public/platform/WebVector.h"
#include "public/platform/modules/indexeddb/WebIDBKey.h"
#include "public/platform/modules/indexeddb/WebIDBKeyPath.h"

namespace blink {

struct WebIDBValue {
  WebIDBValue(const WebData& data, const WebVector<WebBlobInfo>& blob_info)
      : data(data),
        web_blob_info(blob_info),
        primary_key(WebIDBKey::CreateNull()) {}
  WebIDBValue(const WebData& data,
              const WebVector<WebBlobInfo>& blob_info,
              WebIDBKey primary_key,
              const WebIDBKeyPath& key_path)
      : data(data),
        web_blob_info(blob_info),
        primary_key(std::move(primary_key)),
        key_path(key_path) {}

  WebIDBValue() noexcept : primary_key(WebIDBKey::CreateNull()) {}

  WebIDBValue(WebIDBValue&&) noexcept = default;
  WebIDBValue& operator=(WebIDBValue&&) noexcept = default;

  // The serialized JavaScript bits (ignoring blob data) for this IDB Value.
  // Required value.
  WebData data;
  // Collection of blob info referenced by [[data]]. Optional and empty for
  // values without blobs.
  WebVector<WebBlobInfo> web_blob_info;
  // The auto-generated primary key and key path. Both are set when IDB is
  // generating keys (and not JavaScript).  Optional; If set then a property
  // named [[keyPath]] will be set to [[primaryKey]] on the deserialized
  // [[data]] object before calling the event handler.
  WebIDBKey primary_key;
  WebIDBKeyPath key_path;

 private:
  // WebIDBValue has to be move-only, because WebIDBKey is move-only. Making the
  // restriction explicit results in slightly better compilation error messages
  // in code that attempts copying.
  WebIDBValue(WebIDBValue&) = delete;
  WebIDBValue& operator=(WebIDBValue&) = delete;
};

}  // namespace blink

#endif
