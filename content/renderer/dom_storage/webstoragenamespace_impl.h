// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_DOM_STORAGE_WEBSTORAGENAMESPACE_IMPL_H_
#define CONTENT_RENDERER_DOM_STORAGE_WEBSTORAGENAMESPACE_IMPL_H_

#include <stdint.h>
#include <string>

#include "third_party/WebKit/public/platform/WebStorageNamespace.h"

namespace content {

class WebStorageNamespaceImpl : public blink::WebStorageNamespace {
 public:
  // The default constructor creates a local storage namespace, the second
  // constructor should be used for session storage namepaces.
  WebStorageNamespaceImpl();
  explicit WebStorageNamespaceImpl(const std::string& namespace_id);
  ~WebStorageNamespaceImpl() override;

  // See WebStorageNamespace.h for documentation on these functions.
  blink::WebStorageArea* CreateStorageArea(
      const blink::WebSecurityOrigin& origin) override;
  virtual blink::WebStorageNamespace* copy();
  bool IsSameNamespace(const WebStorageNamespace&) const override;

 private:
  std::string namespace_id_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_DOM_STORAGE_WEBSTORAGENAMESPACE_IMPL_H_
