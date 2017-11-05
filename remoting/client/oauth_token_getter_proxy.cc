// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/client/oauth_token_getter_proxy.h"

#include "base/threading/thread_task_runner_handle.h"

namespace remoting {

OAuthTokenGetterProxy::OAuthTokenGetterProxy(
    std::unique_ptr<OAuthTokenGetter> token_getter,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : token_getter_(std::move(token_getter)),
      task_runner_(task_runner),
      weak_factory_(this) {
  weak_ptr_ = weak_factory_.GetWeakPtr();
}

OAuthTokenGetterProxy::~OAuthTokenGetterProxy() {}

void OAuthTokenGetterProxy::CallWithToken(
    const OAuthTokenGetter::TokenCallback& on_access_token) {
  scoped_refptr<base::SingleThreadTaskRunner> current_task_runner =
      base::ThreadTaskRunnerHandle::Get();

  if (task_runner_->BelongsToCurrentThread()) {
    RequestTokenOnRunnerThread(on_access_token, current_task_runner);
    return;
  }

  task_runner_->PostTask(
      FROM_HERE, base::Bind(&OAuthTokenGetterProxy::RequestTokenOnRunnerThread,
                            weak_ptr_, on_access_token, current_task_runner));
}

void OAuthTokenGetterProxy::InvalidateCache() {
  if (task_runner_->BelongsToCurrentThread()) {
    InvalidateCacheOnRunnerThread();
    return;
  }
  task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&OAuthTokenGetterProxy::InvalidateCacheOnRunnerThread,
                 weak_ptr_));
}

void OAuthTokenGetterProxy::RequestTokenOnRunnerThread(
    const TokenCallback& on_access_token,
    scoped_refptr<base::SingleThreadTaskRunner> original_task_runner) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  token_getter_->CallWithToken(
      base::Bind(&OAuthTokenGetterProxy::ResolveCallback, weak_ptr_,
                 on_access_token, original_task_runner));
}

void OAuthTokenGetterProxy::ResolveCallback(
    const TokenCallback& on_access_token,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    Status status,
    const std::string& user_email,
    const std::string& access_token) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  if (task_runner->BelongsToCurrentThread()) {
    on_access_token.Run(status, user_email, access_token);
    return;
  }

  task_runner->PostTask(
      FROM_HERE, base::Bind(on_access_token, status, user_email, access_token));
}

void OAuthTokenGetterProxy::InvalidateCacheOnRunnerThread() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  token_getter_->InvalidateCache();
}

}  // namespace remoting
