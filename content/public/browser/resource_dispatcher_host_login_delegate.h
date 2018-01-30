// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_RESOURCE_DISPATCHER_HOST_LOGIN_DELEGATE_H_
#define CONTENT_PUBLIC_BROWSER_RESOURCE_DISPATCHER_HOST_LOGIN_DELEGATE_H_

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "net/base/auth.h"

namespace content {

// Interface for getting login credentials for HTTP auth requests. If the
// login delegate obtains credentials, it should call the URLRequest's SetAuth
// method. If the user cancels, the login delegate should call the URLRequest's
// CancelAuth instead. And in either case, it must make a call to
// ResourceDispatcherHost::ClearLoginDelegateForRequest.
class CONTENT_EXPORT ResourceDispatcherHostLoginDelegate
    : public base::RefCountedThreadSafe<ResourceDispatcherHostLoginDelegate> {
 public:
  typedef base::Callback<void(const net::AuthCredentials&)>
      AuthRequiredCallback;

  ResourceDispatcherHostLoginDelegate();

  // Notify the login delegate that the request was cancelled.
  // This function can only be called from the IO thread.
  virtual void OnRequestCancelled() = 0;

  void set_auth_required_callback(
      const AuthRequiredCallback& auth_required_callback) {
    auth_required_callback_ = auth_required_callback;
  }

 protected:
  friend class base::RefCountedThreadSafe<ResourceDispatcherHostLoginDelegate>;
  virtual ~ResourceDispatcherHostLoginDelegate();

  AuthRequiredCallback auth_required_callback_;
};

}  // public content

#endif  // CONTENT_PUBLIC_BROWSER_RESOURCE_DISPATCHER_HOST_LOGIN_DELEGATE_H_
