// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/db_message_filter.h"

#include "content/common/database_messages.h"
#include "storage/common/database/database_identifier.h"
#include "third_party/WebKit/public/platform/WebSecurityOrigin.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/platform/WebURL.h"
#include "third_party/WebKit/public/web/WebDatabase.h"
#include "url/origin.h"

using blink::WebSecurityOrigin;
using blink::WebString;

namespace content {

DBMessageFilter::DBMessageFilter() = default;

DBMessageFilter::~DBMessageFilter() = default;

void DBMessageFilter::UpdateSize(const url::Origin& origin,
                                 const base::string16& name,
                                 int64_t size) {
  DCHECK(!origin.unique());
  blink::WebDatabase::UpdateDatabaseSize(origin, WebString::FromUTF16(name),
                                         size);
}

void DBMessageFilter::UpdateSpaceAvailable(const url::Origin& origin,
                                           int64_t space_available) {
  DCHECK(!origin.unique());
  blink::WebDatabase::UpdateSpaceAvailable(origin, space_available);
}

void DBMessageFilter::ResetSpaceAvailable(const url::Origin& origin) {
  DCHECK(!origin.unique());
  blink::WebDatabase::ResetSpaceAvailable(origin);
}

void DBMessageFilter::CloseImmediately(const url::Origin& origin,
                                       const base::string16& name) {
  DCHECK(!origin.unique());
  blink::WebDatabase::CloseDatabaseImmediately(origin,
                                               WebString::FromUTF16(name));
}

}  // namespace content
