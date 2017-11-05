// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_OAUTH_TOKEN_GETTER_PROXY_H_
#define REMOTING_CLIENT_OAUTH_TOKEN_GETTER_PROXY_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "remoting/base/oauth_token_getter.h"

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace remoting {

class OAuthTokenGetterProxy : public OAuthTokenGetter {
 public:
  OAuthTokenGetterProxy(
      std::unique_ptr<OAuthTokenGetter> token_getter,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);
  ~OAuthTokenGetterProxy() override;

  // OAuthTokenGetter overrides.
  void CallWithToken(const TokenCallback& on_access_token) override;
  void InvalidateCache() override;

 private:
  void RequestTokenOnRunnerThread(
      const TokenCallback& on_access_token,
      scoped_refptr<base::SingleThreadTaskRunner> original_task_runner);

  void ResolveCallback(const TokenCallback& on_access_token,
                       scoped_refptr<base::SingleThreadTaskRunner> task_runner,
                       Status status,
                       const std::string& user_email,
                       const std::string& access_token);

  void InvalidateCacheOnRunnerThread();

  std::unique_ptr<OAuthTokenGetter> token_getter_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  base::WeakPtr<OAuthTokenGetterProxy> weak_ptr_;
  base::WeakPtrFactory<OAuthTokenGetterProxy> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(OAuthTokenGetterProxy);
};

}  // namespace remoting

#endif  // REMOTING_CLIENT_OAUTH_TOKEN_GETTER_PROXY_H_
