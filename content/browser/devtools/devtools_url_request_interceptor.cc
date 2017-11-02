// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/devtools_url_request_interceptor.h"

#include "base/memory/ptr_util.h"
#include "base/strings/pattern.h"
#include "base/strings/stringprintf.h"
#include "base/supports_user_data.h"
#include "content/browser/devtools/devtools_agent_host_impl.h"
#include "content/browser/devtools/devtools_url_interceptor_request_job.h"
#include "content/browser/devtools/protocol/network_handler.h"
#include "content/browser/frame_host/frame_tree_node.h"
#include "content/browser/web_contents/web_contents_impl.h"
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

}  // namespace

InterceptedRequestInfo::InterceptedRequestInfo() = default;
InterceptedRequestInfo::~InterceptedRequestInfo() = default;

struct DevToolsURLRequestInterceptor::FilterEntry {
  FilterEntry(
      const base::UnguessableToken& target_id,
      std::vector<Pattern> patterns,
      DevToolsURLRequestInterceptor::RequestInterceptedCallback callback)
      : target_id(target_id),
        patterns(std::move(patterns)),
        callback(std::move(callback)) {}
  FilterEntry(FilterEntry&&) = default;

  void set_patterns(std::vector<Pattern> val) { patterns = std::move(val); }

  const base::UnguessableToken target_id;
  std::vector<Pattern> patterns;
  const DevToolsURLRequestInterceptor::RequestInterceptedCallback callback;

  DISALLOW_COPY_AND_ASSIGN(FilterEntry);
};

// static
bool DevToolsURLRequestInterceptor::IsNavigationRequest(
    ResourceType resource_type) {
  return resource_type == RESOURCE_TYPE_MAIN_FRAME ||
         resource_type == RESOURCE_TYPE_SUB_FRAME;
}

DevToolsURLRequestInterceptor::DevToolsURLRequestInterceptor(
    BrowserContext* browser_context)
    : weak_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  state_ = new State(weak_factory_.GetWeakPtr());
  browser_context->SetUserData(
      kDevToolsURLRequestInterceptorKeyName,
      std::make_unique<DevToolsURLRequestInterceptorUserData>(this));
}

DevToolsURLRequestInterceptor::~DevToolsURLRequestInterceptor() {
  // The BrowserContext owns us, so we don't need to unregister
  // DevToolsURLRequestInterceptorUserData explicitly.
}

net::URLRequestJob* DevToolsURLRequestInterceptor::MaybeInterceptRequest(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return state_->MaybeCreateDevToolsURLInterceptorRequestJob(request,
                                                             network_delegate);
}

std::unique_ptr<InterceptionHandle>
DevToolsURLRequestInterceptor::StartInterceptingRequests(
    const FrameTreeNode* target_frame,
    std::vector<Pattern> intercepted_patterns,
    RequestInterceptedCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  const base::UnguessableToken& target_id =
      target_frame->devtools_frame_token();

  auto filter_entry = std::make_unique<FilterEntry>(
      target_id, std::move(intercepted_patterns), std::move(callback));
  DevToolsTargetRegistry::RegistrationHandle registration_handle =
      state_->target_registry_->RegisterWebContents(
          WebContentsImpl::FromFrameTreeNode(target_frame));
  std::unique_ptr<InterceptionHandle> handle(new InterceptionHandle(
      std::move(registration_handle), state_, filter_entry.get()));

  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::BindOnce(&State::AddFilterEntryOnIO, state_,
                                         std::move(filter_entry)));
  return handle;
}

void DevToolsURLRequestInterceptor::ContinueInterceptedRequest(
    std::string interception_id,
    std::unique_ptr<Modifications> modifications,
    std::unique_ptr<ContinueInterceptedRequestCallback> callback) {
  if (modifications->mark_as_canceled &&
      *modifications->error_reason == net::ERR_CONNECTION_ABORTED) {
    auto it = navigation_requests_.find(interception_id);
    if (it != navigation_requests_.end()) {
      canceled_navigation_requests_.insert(it->second);
      // To successfully cancel navigation the request must succeed. We
      // provide simple mock response to avoid pointless network fetch.
      modifications->error_reason.reset();
      modifications->raw_response = std::string("HTTP/1.1 200 OK\r\n\r\n");
    }
  }

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          &DevToolsURLRequestInterceptor::State::ContinueInterceptedRequestOnIO,
          state_, interception_id, base::Passed(std::move(modifications)),
          base::Passed(std::move(callback))));
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

DevToolsURLRequestInterceptor::Pattern::Pattern() = default;

DevToolsURLRequestInterceptor::Pattern::~Pattern() = default;

DevToolsURLRequestInterceptor::Pattern::Pattern(const Pattern& other) = default;

DevToolsURLRequestInterceptor::Pattern::Pattern(
    const std::string& url_pattern,
    base::flat_set<ResourceType> resource_types)
    : url_pattern(url_pattern), resource_types(std::move(resource_types)) {}

DevToolsURLRequestInterceptor::State::State(
    base::WeakPtr<DevToolsURLRequestInterceptor> interceptor)
    : target_registry_(new DevToolsTargetRegistry(
          content::BrowserThread::GetTaskRunnerForThread(
              content::BrowserThread::IO))),
      next_id_(0),
      interceptor_(std::move(interceptor)) {}

DevToolsURLRequestInterceptor::State::~State() {}

const DevToolsTargetRegistry::TargetInfo*
DevToolsURLRequestInterceptor::State::TargetInfoForRequestInfo(
    const ResourceRequestInfo* request_info) {
  int frame_node_id = request_info->GetFrameTreeNodeId();
  if (frame_node_id != -1)
    return target_registry_->GetInfoByFrameTreeNodeId(frame_node_id);
  return target_registry_->GetInfoByRenderFramePair(
      request_info->GetChildID(), request_info->GetRenderFrameID());
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

DevToolsURLRequestInterceptor::FilterEntry*
DevToolsURLRequestInterceptor::State::FilterEntryForRequest(
    const base::UnguessableToken target_id,
    const GURL& url,
    ResourceType resource_type) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  auto it = target_id_to_entries_.find(target_id);
  if (it == target_id_to_entries_.end())
    return nullptr;

  const std::vector<std::unique_ptr<FilterEntry>>& entries = it->second;
  const std::string url_str = protocol::NetworkHandler::ClearUrlRef(url).spec();
  for (const auto& entry : entries) {
    for (const Pattern& pattern : entry->patterns) {
      if (!pattern.resource_types.empty() &&
          pattern.resource_types.find(resource_type) ==
              pattern.resource_types.end()) {
        continue;
      }
      if (base::MatchPattern(url_str, pattern.url_pattern))
        return entry.get();
    }
  }
  return nullptr;
}

DevToolsURLInterceptorRequestJob* DevToolsURLRequestInterceptor::State::
    MaybeCreateDevToolsURLInterceptorRequestJob(
        net::URLRequest* request,
        net::NetworkDelegate* network_delegate) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  // Bail out if we're not intercepting anything.
  if (target_id_to_entries_.empty())
    return nullptr;
  // Don't try to intercept blob resources.
  if (request->url().SchemeIsBlob())
    return nullptr;
  const ResourceRequestInfo* resource_request_info =
      ResourceRequestInfo::ForRequest(request);
  if (!resource_request_info)
    return nullptr;
  ResourceType resource_type = resource_request_info->GetResourceType();
  const DevToolsTargetRegistry::TargetInfo* target_info =
      TargetInfoForRequestInfo(resource_request_info);
  if (!target_info)
    return nullptr;

  // We don't want to intercept our own sub requests.
  if (sub_requests_.find(request) != sub_requests_.end())
    return nullptr;

  FilterEntry* entry = FilterEntryForRequest(target_info->devtools_target_id,
                                             request->url(), resource_type);
  if (!entry)
    return nullptr;

  bool is_redirect;
  std::string interception_id = GetIdForRequestOnIO(request, &is_redirect);

  if (IsNavigationRequest(resource_type)) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(&DevToolsURLRequestInterceptor::NavigationStarted,
                       interceptor_, interception_id,
                       resource_request_info->GetGlobalRequestID()));
  }

  DevToolsURLInterceptorRequestJob* job = new DevToolsURLInterceptorRequestJob(
      this, interception_id, reinterpret_cast<int64_t>(entry), request,
      network_delegate, target_info->devtools_token, entry->callback,
      is_redirect, resource_request_info->GetResourceType());
  interception_id_to_job_map_[interception_id] = job;
  return job;
}

void DevToolsURLRequestInterceptor::State::AddFilterEntryOnIO(
    std::unique_ptr<FilterEntry> entry) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  const base::UnguessableToken& target_id = entry->target_id;
  auto it = target_id_to_entries_.find(target_id);
  if (it == target_id_to_entries_.end())
    it = target_id_to_entries_.emplace(target_id, 0).first;
  it->second.push_back(std::move(entry));
}

void DevToolsURLRequestInterceptor::State::RemoveFilterEntryOnIO(
    const FilterEntry* entry) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  for (const auto pair : interception_id_to_job_map_) {
    if (pair.second->owning_entry_id() == reinterpret_cast<intptr_t>(entry))
      pair.second->StopIntercepting();
  }

  auto it = target_id_to_entries_.find(entry->target_id);
  if (it == target_id_to_entries_.end())
    return;
  base::EraseIf(it->second, [entry](const std::unique_ptr<FilterEntry>& e) {
    return e.get() == entry;
  });
  if (it->second.empty())
    target_id_to_entries_.erase(it);
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
    const std::string& interception_id,
    bool is_navigation) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  interception_id_to_job_map_.erase(interception_id);
  if (!is_navigation)
    return;
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&DevToolsURLRequestInterceptor::NavigationFinished,
                     interceptor_, interception_id));
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

bool DevToolsURLRequestInterceptor::ShouldCancelNavigation(
    const GlobalRequestID& global_request_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto it = canceled_navigation_requests_.find(global_request_id);
  if (it == canceled_navigation_requests_.end())
    return false;
  canceled_navigation_requests_.erase(it);
  return true;
}

void DevToolsURLRequestInterceptor::NavigationStarted(
    const std::string& interception_id,
    const GlobalRequestID& request_id) {
  navigation_requests_[interception_id] = request_id;
}

void DevToolsURLRequestInterceptor::NavigationFinished(
    const std::string& interception_id) {
  navigation_requests_.erase(interception_id);
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

InterceptionHandle::InterceptionHandle(
    DevToolsTargetRegistry::RegistrationHandle registration,
    scoped_refptr<DevToolsURLRequestInterceptor::State> state,
    DevToolsURLRequestInterceptor::FilterEntry* entry)
    : registration_(registration), state_(state), entry_(entry) {}

InterceptionHandle::~InterceptionHandle() {
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          &DevToolsURLRequestInterceptor::State::RemoveFilterEntryOnIO, state_,
          entry_));
}

void InterceptionHandle::UpdatePatterns(
    std::vector<DevToolsURLRequestInterceptor::Pattern> patterns) {
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&DevToolsURLRequestInterceptor::FilterEntry::set_patterns,
                     base::Unretained(entry_), std::move(patterns)));
}

}  // namespace content
