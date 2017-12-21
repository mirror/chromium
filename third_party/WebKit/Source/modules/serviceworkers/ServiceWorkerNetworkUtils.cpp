// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/serviceworkers/ServiceWorkerNetworkUtils.h"

#include "core/dom/ExecutionContext.h"
#include "modules/fetch/BlobBytesConsumer.h"
#include "modules/fetch/BodyStreamBuffer.h"
#include "modules/fetch/FetchHeaderList.h"
#include "modules/fetch/FetchRequestData.h"
#include "modules/fetch/FormDataBytesConsumer.h"
#include "modules/fetch/Request.h"
#include "platform/weborigin/SecurityOrigin.h"

namespace blink {

Request* ServiceWorkerNetworkUtils::ToRequest(
    ScriptState* script_state,
    const WebServiceWorkerRequest& web_request) {
  FetchRequestData* request = FetchRequestData::Create();
  request->SetURL(web_request.Url());
  request->SetMethod(web_request.Method());
  for (HTTPHeaderMap::const_iterator it = web_request.Headers().begin();
       it != web_request.Headers().end(); ++it)
    request->HeaderList()->Append(it->key, it->value);
  if (scoped_refptr<EncodedFormData> body = web_request.Body()) {
    request->SetBuffer(new BodyStreamBuffer(
        script_state,
        new FormDataBytesConsumer(ExecutionContext::From(script_state),
                                  std::move(body))));
  } else if (web_request.GetBlobDataHandle()) {
    request->SetBuffer(new BodyStreamBuffer(
        script_state,
        new BlobBytesConsumer(ExecutionContext::From(script_state),
                              web_request.GetBlobDataHandle())));
  }
  request->SetContext(web_request.GetRequestContext());
  request->SetReferrer(
      Referrer(web_request.ReferrerUrl().GetString(),
               static_cast<ReferrerPolicy>(web_request.GetReferrerPolicy())));
  request->SetMode(web_request.Mode());
  request->SetCredentials(web_request.CredentialsMode());
  request->SetCacheMode(web_request.CacheMode());
  request->SetRedirect(web_request.RedirectMode());
  request->SetMIMEType(request->HeaderList()->ExtractMIMEType());
  request->SetIntegrity(web_request.Integrity());
  request->SetKeepalive(web_request.Keepalive());
  return Request::Create(script_state, request);
}

}  // namespace blink
