// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SHARED_WORKER_SHARED_WORKER_INSTANCE_H_
#define CONTENT_BROWSER_SHARED_WORKER_SHARED_WORKER_INSTANCE_H_

#include <string>

#include "content/common/content_export.h"
#include "content/common/shared_worker/shared_worker_info.mojom.h"
#include "third_party/WebKit/public/platform/WebAddressSpace.h"
#include "third_party/WebKit/public/platform/WebContentSecurityPolicy.h"
#include "third_party/WebKit/public/web/shared_worker_creation_context_type.mojom.h"
#include "url/gurl.h"

namespace content {

// SharedWorkerInstance is copyable value-type data type. It could be passed to
// the UI thread and be used for comparison in SharedWorkerDevToolsManager.
class CONTENT_EXPORT SharedWorkerInstance {
 public:
  SharedWorkerInstance(
      mojom::SharedWorkerInfoPtr info,
      blink::mojom::SharedWorkerCreationContextType creation_context_type);
  SharedWorkerInstance(const SharedWorkerInstance& other);
  ~SharedWorkerInstance();

  // Checks if this SharedWorkerInstance matches the passed url/name params
  // based on the algorithm in the WebWorkers spec - an instance matches if the
  // origins of the URLs match, and:
  // a) the names are non-empty and equal.
  // -or-
  // b) the names are both empty, and the urls are equal.
  bool Matches(const GURL& url, const std::string& name) const;
  bool Matches(const SharedWorkerInstance& other) const;

  // Accessors.
  const GURL& url() const { return url_; }
  const std::string name() const { return name_; }
  const std::string content_security_policy() const {
    return content_security_policy_;
  }
  blink::WebContentSecurityPolicyType content_security_policy_type() const {
    return content_security_policy_type_;
  }
  blink::WebAddressSpace creation_address_space() const {
    return creation_address_space_;
  }
  blink::mojom::SharedWorkerCreationContextType creation_context_type() const {
    return creation_context_type_;
  }
  bool data_saver_enabled() const { return data_saver_enabled_; }

 private:
  const GURL url_;
  const std::string name_;
  const std::string content_security_policy_;
  const blink::WebContentSecurityPolicyType content_security_policy_type_;
  const blink::WebAddressSpace creation_address_space_;
  const blink::mojom::SharedWorkerCreationContextType creation_context_type_;
  const bool data_saver_enabled_;
};

}  // namespace content


#endif  // CONTENT_BROWSER_SHARED_WORKER_SHARED_WORKER_INSTANCE_H_
