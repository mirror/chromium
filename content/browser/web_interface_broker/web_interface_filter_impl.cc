// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/web_interface_filter.h"

#include <memory>
#include <utility>

#include "base/containers/flat_set.h"
#include "content/public/browser/web_context_type.h"

namespace url {
class Origin;
}

namespace content {
namespace {

class AlwaysAllowFilter : public WebInterfaceFilter {
 public:
  Result FilterInterface(WebContextType context_type,
                         RenderProcessHost* process_host,
                         RenderFrameHost* frame_host,
                         const url::Origin& origin) override {
    return Result::kAllow;
  }
};

WebInterfaceFilter::Result ResolveResults(WebInterfaceFilter::Result a,
                                          WebInterfaceFilter::Result b) {
  if (a == WebInterfaceFilter::Result::kBadMessage ||
      b == WebInterfaceFilter::Result::kBadMessage) {
    return WebInterfaceFilter::Result::kBadMessage;
  }
  if (a == WebInterfaceFilter::Result::kDeny ||
      b == WebInterfaceFilter::Result::kDeny) {
    return WebInterfaceFilter::Result::kDeny;
  }
  return WebInterfaceFilter::Result::kAllow;
}

class WebFilterBundle : public WebInterfaceFilter {
 public:
  WebFilterBundle(std::vector<std::unique_ptr<WebInterfaceFilter>> filters)
      : filters_(std::move(filters)) {}

  Result FilterInterface(WebContextType type,
                         RenderProcessHost* process_host,
                         RenderFrameHost* frame_host,
                         const url::Origin& origin) override {
    auto result = WebInterfaceFilter::Result::kAllow;
    for (auto& filter : filters_) {
      result = ResolveResults(
          result,
          filter->FilterInterface(type, process_host, frame_host, origin));
      if (result == WebInterfaceFilter::Result::kBadMessage) {
        return WebInterfaceFilter::Result::kBadMessage;
      }
    }
    return result;
  }

 private:
  std::vector<std::unique_ptr<WebInterfaceFilter>> filters_;
};

class ContextTypeInterfaceFilter : public WebInterfaceFilter {
 public:
  explicit ContextTypeInterfaceFilter(
      std::initializer_list<WebContextType> allowed_contexts)
      : context_types_(allowed_contexts, base::KEEP_FIRST_OF_DUPES) {}

  Result FilterInterface(WebContextType context_type,
                         RenderProcessHost* process_host,
                         RenderFrameHost* frame_host,
                         const url::Origin& origin) override {
    return context_types_.find(context_type) != context_types_.end()
               ? Result::kAllow
               : Result::kBadMessage;
  }

 private:
  base::flat_set<WebContextType> context_types_;
};

}  // namespace

std::unique_ptr<WebInterfaceFilter> CreateAlwaysAllowFilter() {
  return std::make_unique<AlwaysAllowFilter>();
}

std::unique_ptr<WebInterfaceFilter> CreateFilterBundle(
    std::vector<std::unique_ptr<WebInterfaceFilter>> filters) {
  return std::make_unique<WebFilterBundle>(std::move(filters));
}

std::unique_ptr<WebInterfaceFilter> CreateContextTypeInterfaceFilter(
    std::initializer_list<WebContextType> allowed_contexts) {
  return std::make_unique<ContextTypeInterfaceFilter>(allowed_contexts);
}

}  // namespace content
