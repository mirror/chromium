// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/web_database_impl.h"

#include "content/common/database_messages.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "storage/common/database/database_identifier.h"
#include "third_party/WebKit/public/platform/WebSecurityOrigin.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/platform/WebURL.h"
#include "third_party/WebKit/public/web/WebDatabase.h"
#include "url/origin.h"

using blink::WebSecurityOrigin;
using blink::WebString;

namespace content {

WebDatabaseImpl::WebDatabaseImpl() = default;

WebDatabaseImpl::~WebDatabaseImpl() = default;

void WebDatabaseImpl::Create(content::mojom::WebDatabaseRequest request,
                             const service_manager::BindSourceInfo&) {
  mojo::MakeStrongBinding(base::MakeUnique<WebDatabaseImpl>(),
                          std::move(request));
}

void WebDatabaseImpl::UpdateSize(const url::Origin& origin,
                                 const base::string16& name,
                                 int64_t size) {
  DCHECK(!origin.unique());
  blink::WebDatabase::UpdateDatabaseSize(origin, WebString::FromUTF16(name),
                                         size);
}

void WebDatabaseImpl::CloseImmediately(const url::Origin& origin,
                                       const base::string16& name) {
  DCHECK(!origin.unique());
  blink::WebDatabase::CloseDatabaseImmediately(origin,
                                               WebString::FromUTF16(name));
}

}  // namespace content
