// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/loader/FrameLoadRequest.h"

#include "platform/loader/fetch/ResourceRequest.h"
#include "platform/wtf/text/AtomicString.h"
#include "public/platform/WebURLRequest.h"

namespace blink {

FrameLoadRequest::FrameLoadRequest(Document* origin_document)
    : FrameLoadRequest(origin_document, ResourceRequest()) {}

FrameLoadRequest::FrameLoadRequest(Document* origin_document,
                                   const ResourceRequest& resource_request)
    : FrameLoadRequest(origin_document, resource_request, AtomicString()) {}

FrameLoadRequest::FrameLoadRequest(Document* origin_document,
                                   const ResourceRequest& resource_request,
                                   const AtomicString& frame_name)
    : FrameLoadRequest(origin_document,
                       resource_request,
                       frame_name,
                       kCheckContentSecurityPolicy) {}

FrameLoadRequest::FrameLoadRequest(Document* origin_document,
                                   const ResourceRequest& resource_request,
                                   const SubstituteData& substitute_data)
    : FrameLoadRequest(origin_document,
                       resource_request,
                       AtomicString(),
                       substitute_data,
                       kCheckContentSecurityPolicy) {}

FrameLoadRequest::FrameLoadRequest(
    Document* origin_document,
    const ResourceRequest& resource_request,
    const AtomicString& frame_name,
    ContentSecurityPolicyDisposition
        should_check_main_world_content_security_policy)
    : FrameLoadRequest(origin_document,
                       resource_request,
                       frame_name,
                       SubstituteData(),
                       should_check_main_world_content_security_policy) {}

FrameLoadRequest::FrameLoadRequest(
    Document* origin_document,
    const ResourceRequest& resource_request,
    const AtomicString& frame_name,
    const SubstituteData& substitute_data,
    ContentSecurityPolicyDisposition
        should_check_main_world_content_security_policy)
    : origin_document_(origin_document),
      resource_request_(resource_request),
      frame_name_(frame_name),
      substitute_data_(substitute_data),
      replaces_current_item_(false),
      client_redirect_(ClientRedirectPolicy::kNotClientRedirect),
      should_send_referrer_(kMaybeSendReferrer),
      should_set_opener_(kMaybeSetOpener),
      should_check_main_world_content_security_policy_(
          should_check_main_world_content_security_policy) {
  // These flags are passed to a service worker which controls the page.
  resource_request_.SetFetchRequestMode(
      WebURLRequest::kFetchRequestModeNavigate);
  resource_request_.SetFetchCredentialsMode(
      WebURLRequest::kFetchCredentialsModeInclude);
  resource_request_.SetFetchRedirectMode(
      WebURLRequest::kFetchRedirectModeManual);
}

}  // namespace blink
