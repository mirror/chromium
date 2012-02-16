// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/throttling_resource_handler.h"

#include "content/browser/renderer_host/resource_dispatcher_host.h"
#include "content/public/browser/resource_throttle.h"
#include "content/public/common/resource_response.h"

namespace content {

ThrottlingResourceHandler::ThrottlingResourceHandler(
    ResourceDispatcherHost* host,
    ResourceHandler* next_handler,
    int child_id,
    int request_id,
    ScopedVector<ResourceThrottle> throttles)
    : LayeredResourceHandler(next_handler),
      deferred_stage_(DEFERRED_NONE),
      host_(host),
      child_id_(child_id),
      request_id_(request_id),
      throttles_(throttles.Pass()),
      index_(0) {
  for (size_t i = 0; i < throttles_.size(); ++i)
    throttles_[i]->set_controller(this);
}

bool ThrottlingResourceHandler::OnRequestRedirected(int request_id,
                                                    const GURL& new_url,
                                                    ResourceResponse* response,
                                                    bool* defer) {
  DCHECK_EQ(request_id_, request_id);

  *defer = false;
  while (index_ < throttles_.size()) {
    throttles_[index_]->WillRedirectRequest(new_url, defer);
    index_++;
    if (*defer) {
      deferred_stage_ = DEFERRED_REDIRECT;
      deferred_url_ = new_url;
      deferred_response_ = response;
      return true;  // Do not cancel.
    }
  }

  index_ = 0;  // Reset for next time.

  return next_handler_->OnRequestRedirected(request_id, new_url, response,
                                            defer);
}

bool ThrottlingResourceHandler::OnWillStart(int request_id,
                                            const GURL& url,
                                            bool* defer) {
  DCHECK_EQ(request_id_, request_id);

  *defer = false;
  while (index_ < throttles_.size()) {
    throttles_[index_]->WillStartRequest(defer);
    index_++;
    if (*defer) {
      deferred_stage_ = DEFERRED_START;
      deferred_url_ = url;
      return true;  // Do not cancel.
    }
  }

  index_ = 0;  // Reset for next time.

  return next_handler_->OnWillStart(request_id, url, defer);
}

bool ThrottlingResourceHandler::OnResponseStarted(
    int request_id,
    content::ResourceResponse* response) {
  DCHECK_EQ(request_id_, request_id);

  while (index_ < throttles_.size()) {
    bool defer = false;
    throttles_[index_]->WillProcessResponse(&defer);
    index_++;
    if (defer) {
      host_->PauseRequest(child_id_, request_id_, true);
      deferred_stage_ = DEFERRED_RESPONSE;
      deferred_response_ = response;
      return true;  // Do not cancel.
    }
  }

  index_ = 0;  // Reset for next time.

  return next_handler_->OnResponseStarted(request_id, response);
}

void ThrottlingResourceHandler::OnRequestClosed() {
  throttles_.reset();
  next_handler_->OnRequestClosed();
}

void ThrottlingResourceHandler::Cancel() {
  host_->CancelRequest(child_id_, request_id_, false);
}

void ThrottlingResourceHandler::Resume() {
  switch (deferred_stage_) {
    case DEFERRED_NONE:
      NOTREACHED();
      break;
    case DEFERRED_START:
      ResumeStart();
      break;
    case DEFERRED_REDIRECT:
      ResumeRedirect();
      break;
    case DEFERRED_RESPONSE:
      ResumeResponse();
      break;
  }
  deferred_stage_ = DEFERRED_NONE;
}

ThrottlingResourceHandler::~ThrottlingResourceHandler() {
}

void ThrottlingResourceHandler::ResumeStart() {
  GURL url = deferred_url_;
  deferred_url_ = GURL();

  bool defer = false;
  if (!OnWillStart(request_id_, url, &defer)) {
    Cancel();
  } else if (!defer) {
    host_->StartDeferredRequest(child_id_, request_id_);
  }
}

void ThrottlingResourceHandler::ResumeRedirect() {
  GURL new_url = deferred_url_;
  deferred_url_ = GURL();
  scoped_refptr<ResourceResponse> response;
  deferred_response_.swap(response);

  bool defer = false;
  if (!OnRequestRedirected(request_id_, new_url, response, &defer)) {
    Cancel();
  } else if (!defer) {
    host_->FollowDeferredRedirect(child_id_, request_id_, false, GURL());
  }
}

void ThrottlingResourceHandler::ResumeResponse() {
  scoped_refptr<ResourceResponse> response;
  deferred_response_.swap(response);

  // OnResponseStarted will pause again if necessary.
  host_->PauseRequest(child_id_, request_id_, false);

  if (!OnResponseStarted(request_id_, response))
    Cancel();
}

}  // namespace content
