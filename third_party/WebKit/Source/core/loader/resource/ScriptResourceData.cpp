// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/loader/resource/ScriptResourceData.h"

#include "platform/HTTPNames.h"
#include "platform/network/mime/MIMETypeRegistry.h"

namespace blink {

// static
bool ScriptResourceData::MimeTypeAllowedByNosniff(
    const ResourceResponse& response) {
  return ParseContentTypeOptionsHeader(
             response.HttpHeaderField(HTTPNames::X_Content_Type_Options)) !=
             kContentTypeOptionsNosniff ||
         MIMETypeRegistry::IsSupportedJavaScriptMIMEType(
             response.HttpContentType());
}

AccessControlStatus ScriptResourceData::GetAccessControlStatus() const {
  if (cors_status_ == CORSStatus::kServiceWorkerOpaque)
    return kOpaqueResource;

  if (IsSameOriginOrCORSSuccessful(cors_status_))
    return kSharableCrossOrigin;

  return kNotSharableCrossOrigin;
}

}  // namespace blink
