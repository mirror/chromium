// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/shared_worker/shared_worker_instance.h"

namespace content {

SharedWorkerInstance::SharedWorkerInstance(
    mojom::SharedWorkerInfoPtr info,
    blink::mojom::SharedWorkerCreationContextType creation_context_type)
    : url_(info->url),
      name_(info->name),
      content_security_policy_(info->content_security_policy),
      content_security_policy_type_(info->content_security_policy_type),
      creation_address_space_(info->creation_address_space),
      creation_context_type_(creation_context_type),
      data_saver_enabled_(info->data_saver_enabled) {}

SharedWorkerInstance::SharedWorkerInstance(const SharedWorkerInstance& other)
    : url_(other.url_),
      name_(other.name_),
      content_security_policy_(other.content_security_policy_),
      content_security_policy_type_(other.content_security_policy_type_),
      creation_address_space_(other.creation_address_space_),
      creation_context_type_(other.creation_context_type_),
      data_saver_enabled_(other.data_saver_enabled_) {}

SharedWorkerInstance::~SharedWorkerInstance() {}

bool SharedWorkerInstance::Matches(const GURL& match_url,
                                   const std::string& match_name) const {
  if (url_.GetOrigin() != match_url.GetOrigin())
    return false;

  if (name_ != match_name || url_ != match_url)
    return false;
  return true;
}

bool SharedWorkerInstance::Matches(const SharedWorkerInstance& other) const {
  return Matches(other.url(), other.name());
}

}  // namespace content
