// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/devtools_url_request_interceptor.h"

#include "base/memory/ptr_util.h"
#include "base/strings/pattern.h"
#include "base/strings/stringprintf.h"
#include "base/supports_user_data.h"
#include "content/browser/devtools/devtools_agent_host_impl.h"
#include "content/browser/devtools/devtools_target_registry.h"
#include "content/browser/devtools/devtools_url_interceptor_request_job.h"
#include "content/browser/devtools/protocol/network_handler.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "net/http/http_request_headers.h"
#include "net/url_request/url_request.h"

namespace content {

namespace {
const char kDevToolsURLRequestInterceptorKeyName[] =
    "DevToolsURLRequestInterceptor";

class DevToolsURLRequestInterceptorUserData
    : public base::SupportsUserData::Data {
 public:
  explicit DevToolsURLRequestInterceptorUserData(
      DevToolsURLRequestInterceptor* devtools_url_request_interceptor)
      : devtools_url_request_interceptor_(devtools_url_request_interceptor) {}

  DevToolsURLRequestInterceptor* devtools_url_request_interceptor() const {
    return devtools_url_request_interceptor_;
  }

 private:
  DevToolsURLRequestInterceptor* devtools_url_request_interceptor_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsURLRequestInterceptorUserData);
};

const DevToolsTargetRegistry::TargetInfo* TargetInfoForRequestInfo(
    const ResourceRequestInfo* request_info) {
  int frame_node_id = request_info->GetFrameTreeNodeId();
  if (frame_node_id != -1)
    return DevToolsTargetRegistry::GetInfoByFrameTreeNodeId(frame_node_id);
  return DevToolsTargetRegistry::GetInfoByRenderFramePair(
      request_info->GetChildID(), request_info->GetRenderFrameID());
}

}  // namespace

DevToolsURLRequestInterceptor::DevToolsURLRequestInterceptor(
    BrowserContext* browser_context)
    : browser_context_(browser_context), state_(new State()) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  browser_context_->SetUserData(
      kDevToolsURLRequestInterceptorKeyName,
      base::MakeUnique<DevToolsURLRequestInterceptorUserData>(this));
}

DevToolsURLRequestInterceptor::~DevToolsURLRequestInterceptor() {
  // The BrowserContext owns us, so we don't need to unregister
  // DevToolsURLRequestInterceptorUserData explicitly.
}

net::URLRequestJob* DevToolsURLRequestInterceptor::MaybeInterceptRequest(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return state()->MaybeCreateDevToolsURLInterceptorRequestJob(request,
                                                              network_delegate);
}

net::URLRequestJob* DevToolsURLRequestInterceptor::MaybeInterceptRedirect(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate,
    const GURL& location) const {
  return nullptr;
}

net::URLRequestJob* DevToolsURLRequestInterceptor::MaybeInterceptResponse(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  return nullptr;
}

DevToolsURLRequestInterceptor::State::State() : next_id_(0) {}

DevToolsURLRequestInterceptor::State::~State() {}

void DevToolsURLRequestInterceptor::State::ContinueInterceptedRequest(
    std::string interception_id,
    std::unique_ptr<Modifications> modifications,
    std::unique_ptr<ContinueInterceptedRequestCallback> callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          &DevToolsURLRequestInterceptor::State::ContinueInterceptedRequestOnIO,
          this, interception_id, base::Passed(std::move(modifications)),
          base::Passed(std::move(callback))));
}

void DevToolsURLRequestInterceptor::State::ContinueInterceptedRequestOnIO(
    std::string interception_id,
    std::unique_ptr<Modifications> modifications,
    std::unique_ptr<ContinueInterceptedRequestCallback> callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DevToolsURLInterceptorRequestJob* job = GetJob(interception_id);
  if (!job) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(
            &ContinueInterceptedRequestCallback::sendFailure,
            base::Passed(std::move(callback)),
            protocol::Response::InvalidParams("Invalid InterceptionId.")));
    return;
  }

  job->ContinueInterceptedRequest(std::move(modifications),
                                  std::move(callback));
}

DevToolsURLInterceptorRequestJob* DevToolsURLRequestInterceptor::State::
    MaybeCreateDevToolsURLInterceptorRequestJob(
        net::URLRequest* request,
        net::NetworkDelegate* network_delegate) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  // Bail out if we're not intercepting anything.
  if (host_id_to_intercepted_page_.empty())
    return nullptr;
  // Don't try to intercept blob resources.
  if (request->url().SchemeIsBlob())
    return nullptr;
  const ResourceRequestInfo* resource_request_info =
      ResourceRequestInfo::ForRequest(request);
  if (!resource_request_info)
    return nullptr;
  const DevToolsTargetRegistry::TargetInfo* target_info =
      TargetInfoForRequestInfo(resource_request_info);
  if (!target_info)
    return nullptr;
  auto it = host_id_to_intercepted_page_.find(target_info->devtools_host_id);
  if (it == host_id_to_intercepted_page_.end())
    return nullptr;
  const InterceptedPage& intercepted_page = *it->second;

  // We don't want to intercept our own sub requests.
  if (sub_requests_.find(request) != sub_requests_.end())
    return nullptr;

  // If we are only intercepting certain resource types then bail out if the
  // request is for the wrong type.
  if (!intercepted_page.intercepted_resource_types.empty() &&
      intercepted_page.intercepted_resource_types.find(
          resource_request_info->GetResourceType()) ==
          intercepted_page.intercepted_resource_types.end()) {
    return nullptr;
  }

  bool matchFound = false;
  for (const std::string& pattern : intercepted_page.intercepted_url_patterns) {
    if (base::MatchPattern(request->url().spec(), pattern)) {
      matchFound = true;
      break;
    }
  }
  if (!matchFound)
    return nullptr;

  bool is_redirect;
  std::string interception_id = GetIdForRequestOnIO(request, &is_redirect);
  DevToolsURLInterceptorRequestJob* job = new DevToolsURLInterceptorRequestJob(
      this, interception_id, request, network_delegate,
      target_info->devtools_host_id, intercepted_page.network_handler,
      is_redirect, resource_request_info->GetResourceType());
  interception_id_to_job_map_[interception_id] = job;
  return job;
}

void DevToolsURLRequestInterceptor::State::StartInterceptingRequestsOnIO(
    const base::UnguessableToken& host_id,
    std::unique_ptr<InterceptedPage> interceptedPage) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  host_id_to_intercepted_page_[host_id] = std::move(interceptedPage);
}

void DevToolsURLRequestInterceptor::State::StartInterceptingRequests(
    const base::UnguessableToken& host_id,
    base::WeakPtr<protocol::NetworkHandler> network_handler,
    std::vector<std::string> intercepted_url_patterns,
    base::flat_set<ResourceType> intercepted_resource_types) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  std::unique_ptr<InterceptedPage> intercepted_page(new InterceptedPage(
      std::move(network_handler), std::move(intercepted_url_patterns),
      std::move(intercepted_resource_types)));
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          &DevToolsURLRequestInterceptor::State::StartInterceptingRequestsOnIO,
          this, host_id, std::move(intercepted_page)));
}

void DevToolsURLRequestInterceptor::State::StopInterceptingRequests(
    const base::UnguessableToken& host_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          &DevToolsURLRequestInterceptor::State::StopInterceptingRequestsOnIO,
          this, host_id));
}

void DevToolsURLRequestInterceptor::State::StopInterceptingRequestsOnIO(
    const base::UnguessableToken& host_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  host_id_to_intercepted_page_.erase(host_id);

  // Tell any jobs associated with associated agent host to stop intercepting.
  for (const auto pair : interception_id_to_job_map_) {
    if (pair.second->host_id() == host_id)
      pair.second->StopIntercepting();
  }
}

void DevToolsURLRequestInterceptor::State::RegisterSubRequest(
    const net::URLRequest* sub_request) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(sub_requests_.find(sub_request) == sub_requests_.end());
  sub_requests_.insert(sub_request);
}

void DevToolsURLRequestInterceptor::State::UnregisterSubRequest(
    const net::URLRequest* sub_request) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(sub_requests_.find(sub_request) != sub_requests_.end());
  sub_requests_.erase(sub_request);
}

void DevToolsURLRequestInterceptor::State::ExpectRequestAfterRedirect(
    const net::URLRequest* request,
    std::string id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  expected_redirects_[request] = id;
}

std::string DevToolsURLRequestInterceptor::State::GetIdForRequestOnIO(
    const net::URLRequest* request,
    bool* is_redirect) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  auto find_it = expected_redirects_.find(request);
  if (find_it == expected_redirects_.end()) {
    *is_redirect = false;
    return base::StringPrintf("id-%zu", ++next_id_);
  }
  *is_redirect = true;
  std::string id = find_it->second;
  expected_redirects_.erase(find_it);
  return id;
}

DevToolsURLInterceptorRequestJob* DevToolsURLRequestInterceptor::State::GetJob(
    const std::string& interception_id) const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  const auto it = interception_id_to_job_map_.find(interception_id);
  if (it == interception_id_to_job_map_.end())
    return nullptr;
  return it->second;
}

void DevToolsURLRequestInterceptor::State::JobFinished(
    const std::string& interception_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  interception_id_to_job_map_.erase(interception_id);
}

// static
DevToolsURLRequestInterceptor*
DevToolsURLRequestInterceptor::FromBrowserContext(BrowserContext* context) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto* interceptor_user_data =
      static_cast<DevToolsURLRequestInterceptorUserData*>(
          context->GetUserData(kDevToolsURLRequestInterceptorKeyName));
  if (!interceptor_user_data)
    return nullptr;
  return interceptor_user_data->devtools_url_request_interceptor();
}

DevToolsURLRequestInterceptor::Modifications::Modifications(
    base::Optional<net::Error> error_reason,
    base::Optional<std::string> raw_response,
    protocol::Maybe<std::string> modified_url,
    protocol::Maybe<std::string> modified_method,
    protocol::Maybe<std::string> modified_post_data,
    protocol::Maybe<protocol::Network::Headers> modified_headers,
    protocol::Maybe<protocol::Network::AuthChallengeResponse>
        auth_challenge_response,
    bool mark_as_canceled)
    : error_reason(std::move(error_reason)),
      raw_response(std::move(raw_response)),
      modified_url(std::move(modified_url)),
      modified_method(std::move(modified_method)),
      modified_post_data(std::move(modified_post_data)),
      modified_headers(std::move(modified_headers)),
      auth_challenge_response(std::move(auth_challenge_response)),
      mark_as_canceled(mark_as_canceled) {}

DevToolsURLRequestInterceptor::Modifications::~Modifications() {}

DevToolsURLRequestInterceptor::State::InterceptedPage::InterceptedPage(
    base::WeakPtr<protocol::NetworkHandler> network_handler,
    std::vector<std::string> intercepted_url_patterns,
    base::flat_set<ResourceType> intercepted_resource_types)
    : network_handler(network_handler),
      intercepted_url_patterns(std::move(intercepted_url_patterns)),
      intercepted_resource_types(std::move(intercepted_resource_types)) {}

DevToolsURLRequestInterceptor::State::InterceptedPage::~InterceptedPage() =
    default;

}  // namespace content
