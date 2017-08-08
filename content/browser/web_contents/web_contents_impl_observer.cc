// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_contents/web_contents_impl_observer.h"

#include "content/browser/web_contents/web_contents_impl.h"

namespace content {

WebContentsImplObserver::WebContentsImplObserver(WebContentsImpl* impl)
    : impl_(impl) {
  if (impl_)
    impl_->AddImplObserver(this);
}

WebContentsImplObserver::~WebContentsImplObserver() {
  if (impl_)
    impl_->RemoveImplObserver(this);
}

}  // namespace content
