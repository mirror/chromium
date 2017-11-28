// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/network/NetworkInstrumentation.h"

#include "base/trace_event/trace_event.h"
#include "platform/instrumentation/tracing/TracedValue.h"
#include "platform/loader/fetch/ResourceLoadPriority.h"
#include "platform/loader/fetch/ResourceRequest.h"
#include "public/platform/WebURLRequest.h"

namespace blink {
namespace network_instrumentation {

using blink::TracedValue;
using network_instrumentation::RequestOutcome;

const char kBlinkResourceID[] = "BlinkResourceID";
const char kResourceLoadTitle[] = "ResourceLoad";
const char kResourcePrioritySetTitle[] = "ResourcePrioritySet";
const char kNetInstrumentationCategory[] = TRACE_DISABLED_BY_DEFAULT("network");

const char* RequestOutcomeToString(RequestOutcome outcome) {
  switch (outcome) {
    case RequestOutcome::kSuccess:
      return "Success";
    case RequestOutcome::kFail:
      return "Fail";
    default:
      NOTREACHED();
      // We need to return something to avoid compiler warning.
      return "This should never happen";
  }
}

// Note: network_instrumentation code should do as much work as possible inside
// the arguments of trace macros so that very little instrumentation overhead is
// incurred if the trace category is disabled. See https://crbug.com/669666.

namespace {

std::unique_ptr<TracedValue> ScopedResourceTrackerBeginData(
    const blink::ResourceRequest& request) {
  std::unique_ptr<TracedValue> data = TracedValue::Create();
  data->SetString("url", request.Url().GetString());
  return data;
}

std::unique_ptr<TracedValue> ResourcePrioritySetData(
    blink::ResourceLoadPriority priority) {
  std::unique_ptr<TracedValue> data = TracedValue::Create();
  data->SetInteger("priority", priority);
  return data;
}

std::unique_ptr<TracedValue> EndResourceLoadData(RequestOutcome outcome) {
  std::unique_ptr<TracedValue> data = TracedValue::Create();
  data->SetString("outcome", RequestOutcomeToString(outcome));
  return data;
}

String FrameTypeToString(WebURLRequest::FrameType frame_type) {
  switch (frame_type) {
    case WebURLRequest::kFrameTypeAuxiliary:
      return "auxiliary";
    case WebURLRequest::kFrameTypeNested:
      return "nested";
    case WebURLRequest::kFrameTypeNone:
      return "none";
    case WebURLRequest::kFrameTypeTopLevel:
      return "top-level";
  }
  return "error";
}

std::unique_ptr<TracedValue> EndResourceInfo(Resource* resource,
                                             ResourceTimingInfo* info) {
  std::unique_ptr<TracedValue> data = TracedValue::Create();
  if (resource) {
    data->SetString(
        "resourceType",
        Resource::ResourceTypeToString(
            resource->GetType(), resource->Options().initiator_info.name));
    data->SetInteger("resource.size", resource->size());
    data->SetInteger("resource.encodedSize", resource->EncodedSize());
    data->SetInteger("resource.decodedSize", resource->DecodedSize());
    data->SetInteger("resource.overheadSize", resource->OverheadSize());
    data->SetInteger("resource.stillNeedsLoad", resource->StillNeedsLoad());
    data->SetString("resource.httpContentType", resource->HttpContentType());

    ResourceRequest request = resource->GetResourceRequest();
    data->SetInteger("request.priority", request.Priority());
    data->SetString("request.HttpMethod", request.HttpMethod());
    data->SetBoolean("request.GetKeepalive", request.GetKeepalive());
    data->SetString("request.GetFrameType",
                    FrameTypeToString(request.GetFrameType()));

    ResourceResponse response = resource->GetResponse();
    data->SetString("response.mimeType", response.MimeType());
    data->SetString("response.securityDetailsSubjectName",
                    response.GetSecurityDetails()->subject_name);
    data->SetString("response.url", response.Url());
    data->SetInteger("response.ExpectedContentLength",
                     response.ExpectedContentLength());
    data->SetString("response.TextEncodingName", response.TextEncodingName());
    data->SetBoolean("response.IsMultipart", response.IsMultipart());
    data->SetString("response.AlpnNegotiatedProtocol",
                    response.AlpnNegotiatedProtocol());
  }
  if (info) {
    data->SetBoolean("info.isMainResource", info->IsMainResource());
    data->SetInteger("info.transferSize", info->TransferSize());
    ResourceLoadTiming* timing = info->FinalResponse().GetResourceLoadTiming();
    if (timing) {
      data->SetDouble("timing.pushStart", timing->PushStart());
      data->SetDouble("timing.pushEnd", timing->PushEnd());
      data->SetDouble("ResponseStart",
                      timing->ReceiveHeadersEnd() - timing->RequestTime());
    }
  }

  return data;
}
}  // namespace

ScopedResourceLoadTracker::ScopedResourceLoadTracker(
    unsigned long resource_id,
    const blink::ResourceRequest& request)
    : resource_load_continues_beyond_scope_(false), resource_id_(resource_id) {
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN1(
      kNetInstrumentationCategory, kResourceLoadTitle,
      TRACE_ID_WITH_SCOPE(kBlinkResourceID, TRACE_ID_LOCAL(resource_id)),
      "beginData", ScopedResourceTrackerBeginData(request));
}

ScopedResourceLoadTracker::~ScopedResourceLoadTracker() {
  if (!resource_load_continues_beyond_scope_) {
    EndResourceLoad(resource_id_, nullptr, nullptr, RequestOutcome::kFail);
  }
}

void ScopedResourceLoadTracker::ResourceLoadContinuesBeyondScope() {
  resource_load_continues_beyond_scope_ = true;
}

void ResourcePrioritySet(unsigned long resource_id,
                         blink::ResourceLoadPriority priority) {
  TRACE_EVENT_NESTABLE_ASYNC_INSTANT1(
      kNetInstrumentationCategory, kResourcePrioritySetTitle,
      TRACE_ID_WITH_SCOPE(kBlinkResourceID, TRACE_ID_LOCAL(resource_id)),
      "data", ResourcePrioritySetData(priority));
}

void EndResourceLoad(unsigned long resource_id,
                     Resource* resource,
                     ResourceTimingInfo* timing,
                     RequestOutcome outcome) {
  TRACE_EVENT_NESTABLE_ASYNC_END2(
      kNetInstrumentationCategory, kResourceLoadTitle,
      TRACE_ID_WITH_SCOPE(kBlinkResourceID, TRACE_ID_LOCAL(resource_id)),
      "endData", EndResourceLoadData(outcome), "resourceInfo",
      EndResourceInfo(resource, timing));
}

}  // namespace network_instrumentation
}  // namespace blink
