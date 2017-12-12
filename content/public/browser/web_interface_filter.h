// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_WEB_INTERFACE_FILTER_H_
#define CONTENT_PUBLIC_BROWSER_WEB_INTERFACE_FILTER_H_

#include <memory>
#include <utility>
#include <vector>

#include "base/macros.h"

namespace url {
class Origin;
}

namespace content {

class RenderFrameHost;
class RenderProcessHost;
enum class PermissionType;

enum class WebContextType {
  kFrame,
  kDedicatedWorker,
  kSharedWorker,
  kServiceWorker,
};

class WebInterfaceFilter {
 public:
  enum Result {
    kAllow,
    kDeny,
    kBadMessage,
  };
  WebInterfaceFilter() = default;
  virtual ~WebInterfaceFilter() = default;

  virtual Result FilterInterface(WebContextType context_type,
                                 RenderProcessHost*,
                                 RenderFrameHost*,
                                 const url::Origin& origin) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(WebInterfaceFilter);
};

std::unique_ptr<WebInterfaceFilter> CreatePermissionInterfaceFilter(
    PermissionType type);

std::unique_ptr<WebInterfaceFilter> CreateContextTypeInterfaceFilter(
    std::initializer_list<WebContextType> allowed_contexts);

class WebInterfaceConfiguration {
 public:
  template <typename... Args>
  WebInterfaceConfiguration(Args... args) {
    static_cast<void>(std::initializer_list<int>{
        (filters_.push_back(std::forward<Args>(args)), 0)...});
  }

  WebInterfaceConfiguration(WebInterfaceConfiguration&&);
  WebInterfaceConfiguration& operator=(WebInterfaceConfiguration&&);

  ~WebInterfaceConfiguration();

  WebInterfaceFilter::Result FilterInterface(WebContextType type,
                                             RenderProcessHost* process_host,
                                             RenderFrameHost* frame_host,
                                             const url::Origin& origin);

 private:
  std::vector<std::unique_ptr<WebInterfaceFilter>> filters_;

  DISALLOW_COPY_AND_ASSIGN(WebInterfaceConfiguration);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_WEB_INTERFACE_FILTER_H_
