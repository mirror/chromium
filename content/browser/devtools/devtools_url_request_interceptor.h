// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_URL_REQUEST_INTERCEPTOR_H_
#define CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_URL_REQUEST_INTERCEPTOR_H_

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/unguessable_token.h"
#include "content/browser/devtools/devtools_target_registry.h"
#include "content/browser/devtools/protocol/network.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/resource_type.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request_interceptor.h"

namespace content {

class BrowserContext;
class DevToolsURLInterceptorRequestJob;
class FrameTreeNode;
class InterceptionHandle;
class ResourceRequestInfo;

struct GlobalRequestID;

struct InterceptedRequestInfo {
  InterceptedRequestInfo();
  ~InterceptedRequestInfo();

  std::string interception_id;
  std::unique_ptr<protocol::Network::Request> network_request;
  base::UnguessableToken frame_id;
  ResourceType resource_type;
  bool is_navigation;
  protocol::Maybe<protocol::Object> headers_object;
  protocol::Maybe<int> http_status_code;
  protocol::Maybe<protocol::String> redirect_url;
  protocol::Maybe<protocol::Network::AuthChallenge> auth_challenge;
};

// An interceptor that creates DevToolsURLInterceptorRequestJobs for requests
// from pages where interception has been enabled via
// Network.setRequestInterceptionEnabled.
class DevToolsURLRequestInterceptor : public net::URLRequestInterceptor {
 private:
  struct FilterEntry;

 public:
  using RequestInterceptedCallback =
      base::Callback<void(std::unique_ptr<InterceptedRequestInfo>)>;

  static bool IsNavigationRequest(ResourceType resource_type);

  explicit DevToolsURLRequestInterceptor(BrowserContext* browser_context);
  ~DevToolsURLRequestInterceptor() override;

  using ContinueInterceptedRequestCallback =
      protocol::Network::Backend::ContinueInterceptedRequestCallback;

  // Must be called on UI thread.
  static DevToolsURLRequestInterceptor* FromBrowserContext(
      BrowserContext* context);

  // net::URLRequestInterceptor implementation:
  net::URLRequestJob* MaybeInterceptRequest(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;

  net::URLRequestJob* MaybeInterceptRedirect(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate,
      const GURL& location) const override;

  net::URLRequestJob* MaybeInterceptResponse(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;

  struct Modifications {
    Modifications(base::Optional<net::Error> error_reason,
                  base::Optional<std::string> raw_response,
                  protocol::Maybe<std::string> modified_url,
                  protocol::Maybe<std::string> modified_method,
                  protocol::Maybe<std::string> modified_post_data,
                  protocol::Maybe<protocol::Network::Headers> modified_headers,
                  protocol::Maybe<protocol::Network::AuthChallengeResponse>
                      auth_challenge_response,
                  bool mark_as_canceled);
    ~Modifications();

    bool RequestShouldContineUnchanged() const;

    // If none of the following are set then the request will be allowed to
    // continue unchanged.
    base::Optional<net::Error> error_reason;   // Finish with error.
    base::Optional<std::string> raw_response;  // Finish with mock response.

    // Optionally modify before sending to network.
    protocol::Maybe<std::string> modified_url;
    protocol::Maybe<std::string> modified_method;
    protocol::Maybe<std::string> modified_post_data;
    protocol::Maybe<protocol::Network::Headers> modified_headers;

    // AuthChallengeResponse is mutually exclusive with the above.
    protocol::Maybe<protocol::Network::AuthChallengeResponse>
        auth_challenge_response;

    bool mark_as_canceled;
  };

  struct Pattern {
   public:
    Pattern();
    ~Pattern();
    Pattern(const Pattern& other);
    Pattern(const std::string& url_pattern,
            base::flat_set<ResourceType> resource_types);
    const std::string url_pattern;
    const base::flat_set<ResourceType> resource_types;
  };

  // The State needs to be accessed on both UI and IO threads.
  class State : public base::RefCountedThreadSafe<State> {
   public:
    // Returns a DevToolsURLInterceptorRequestJob if the request should be
    // intercepted, otherwise returns nullptr. Must be called on the IO thread.
    DevToolsURLInterceptorRequestJob*
    MaybeCreateDevToolsURLInterceptorRequestJob(
        net::URLRequest* request,
        net::NetworkDelegate* network_delegate);

    // Registers a |sub_request| that should not be intercepted.
    void RegisterSubRequest(const net::URLRequest* sub_request);

    // Unregisters a |sub_request|. Must be called on the IO thread.
    void UnregisterSubRequest(const net::URLRequest* sub_request);

    // To make the user's life easier we make sure requests in a redirect chain
    // all have the same id. Must be called on the IO thread.
    void ExpectRequestAfterRedirect(const net::URLRequest* request,
                                    std::string id);

    // Must be called on the IO thread.
    void JobFinished(const std::string& interception_id, bool is_navigation);

   private:
    friend class DevToolsURLRequestInterceptor;
    friend class InterceptionHandle;
    friend class base::RefCountedThreadSafe<State>;

    State();
    ~State();

    const DevToolsTargetRegistry::TargetInfo* TargetInfoForRequestInfo(
        const ResourceRequestInfo* request_info);

    void ContinueInterceptedRequestOnIO(
        std::string interception_id,
        std::unique_ptr<DevToolsURLRequestInterceptor::Modifications>
            modifications,
        std::unique_ptr<ContinueInterceptedRequestCallback> callback);

    void AddFilterEntryOnIO(std::unique_ptr<FilterEntry> entry);
    void RemoveFilterEntryOnIO(const FilterEntry* entry);

    FilterEntry* FilterEntryForRequest(const base::UnguessableToken target_id,
                                       const GURL& url,
                                       ResourceType resource_type);

    // UI thread methods.
    void ContinueInterceptedRequestOnUI(
        std::string interception_id,
        std::unique_ptr<Modifications> modifications,
        std::unique_ptr<ContinueInterceptedRequestCallback> callback);
    void NavigationStartedOnUI(const std::string& interception_id,
                               const GlobalRequestID& request_id);
    void NavigationFinishedOnUI(const std::string& interception_id);
    bool ShouldCancelNavigation(const GlobalRequestID& global_request_id);

    std::string GetIdForRequestOnIO(const net::URLRequest* request,
                                    bool* is_redirect);

    // Returns a WeakPtr to the DevToolsURLInterceptorRequestJob corresponding
    // to |interception_id|.  Must be called on the IO thread.
    DevToolsURLInterceptorRequestJob* GetJob(
        const std::string& interception_id) const;

    std::unique_ptr<DevToolsTargetRegistry> target_registry_;

    base::flat_map<base::UnguessableToken,
                   std::vector<std::unique_ptr<FilterEntry>>>
        target_id_to_entries_;

    base::flat_map<std::string, DevToolsURLInterceptorRequestJob*>
        interception_id_to_job_map_;

    // The DevToolsURLInterceptorRequestJob proxies a sub request to actually
    // fetch the bytes from the network. We don't want to intercept those
    // requests.
    base::flat_set<const net::URLRequest*> sub_requests_;

    // To simplify handling of redirect chains for the end user, we arrange for
    // all requests in the chain to have the same interception id.
    base::flat_map<const net::URLRequest*, std::string> expected_redirects_;
    size_t next_id_;

    // UI thread state
    base::flat_map<std::string, GlobalRequestID> navigation_requests_;
    base::flat_set<GlobalRequestID> canceled_navigation_requests_;

    DISALLOW_COPY_AND_ASSIGN(State);
  };

  // Must be called on the UI thread.
  std::unique_ptr<InterceptionHandle> StartInterceptingRequests(
      const FrameTreeNode* target_frame,
      std::vector<Pattern> intercepted_patterns,
      RequestInterceptedCallback callback);

  // Must be called on the UI thread.
  void ContinueInterceptedRequest(
      std::string interception_id,
      std::unique_ptr<Modifications> modifications,
      std::unique_ptr<ContinueInterceptedRequestCallback> callback);

  bool ShouldCancelNavigation(const GlobalRequestID& global_request_id) {
    return state_->ShouldCancelNavigation(global_request_id);
  }

 private:
  friend class InterceptionHandle;

  scoped_refptr<State> state_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsURLRequestInterceptor);
};

class InterceptionHandle {
 public:
  ~InterceptionHandle();
  void UpdatePatterns(std::vector<DevToolsURLRequestInterceptor::Pattern>);

 private:
  friend class DevToolsURLRequestInterceptor;
  InterceptionHandle(DevToolsTargetRegistry::RegistrationHandle registration,
                     scoped_refptr<DevToolsURLRequestInterceptor::State> state,
                     DevToolsURLRequestInterceptor::FilterEntry* entry);

  DevToolsTargetRegistry::RegistrationHandle registration_;
  scoped_refptr<DevToolsURLRequestInterceptor::State> state_;
  DevToolsURLRequestInterceptor::FilterEntry* entry_;

  DISALLOW_COPY_AND_ASSIGN(InterceptionHandle);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_URL_REQUEST_INTERCEPTOR_H_
