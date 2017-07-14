// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/workers/GlobalScopeStartupData.h"

#include <memory>
#include "platform/network/ContentSecurityPolicyParsers.h"
#include "platform/wtf/PtrUtil.h"

namespace blink {

GlobalScopeStartupData::GlobalScopeStartupData(
    const KURL& script_url,
    const String& user_agent,
    const String& source_code,
    std::unique_ptr<Vector<char>> cached_meta_data,
    WorkerThreadStartMode start_mode,
    const Vector<CSPHeaderAndType>* content_security_policy_headers,
    const String& referrer_policy,
    const SecurityOrigin* starter_origin,
    WorkerClients* worker_clients,
    WebAddressSpace address_space,
    const Vector<String>* origin_trial_tokens,
    std::unique_ptr<WorkerSettings> worker_settings,
    V8CacheOptions v8_cache_options)
    : script_url_(script_url.Copy()),
      user_agent_(user_agent.IsolatedCopy()),
      source_code_(source_code.IsolatedCopy()),
      cached_meta_data_(std::move(cached_meta_data)),
      start_mode_(start_mode),
      referrer_policy_(referrer_policy.IsolatedCopy()),
      starter_origin_privilege_data_(
          starter_origin ? starter_origin->CreatePrivilegeData() : nullptr),
      worker_clients_(worker_clients),
      address_space_(address_space),
      worker_settings_(std::move(worker_settings)),
      v8_cache_options(v8_cache_options) {
  content_security_policy_headers_ =
      WTF::MakeUnique<Vector<CSPHeaderAndType>>();
  if (content_security_policy_headers) {
    for (const auto& header : *content_security_policy_headers) {
      CSPHeaderAndType copied_header(header.first.IsolatedCopy(),
                                     header.second);
      content_security_policy_headers_->push_back(copied_header);
    }
  }

  origin_trial_tokens_ = std::unique_ptr<Vector<String>>(new Vector<String>());
  if (origin_trial_tokens) {
    for (const String& token : *origin_trial_tokens)
      origin_trial_tokens_->push_back(token.IsolatedCopy());
  }
}

}  // namespace blink
