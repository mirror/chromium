// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/keyboard_lock/keyboard_lock_service_impl.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace {
// These values must stay in sync with tools/metrics/histograms.xml.
enum KeyboardLockMethods {
  kRequestAllKeys = 0,
  kRequestSomeKeys = 1,
  kCancelLock = 2,
  kMaxValue = 3
};
}  // namespace

namespace content {

KeyboardLockServiceImpl::KeyboardLockServiceImpl(
    RenderFrameHost* render_frame_host)
    : web_contents_(WebContents::FromRenderFrameHost(render_frame_host)) {
  DCHECK(web_contents_);
}

KeyboardLockServiceImpl::~KeyboardLockServiceImpl() = default;

// static
void KeyboardLockServiceImpl::CreateMojoService(
    RenderFrameHost* render_frame_host,
    blink::mojom::KeyboardLockServiceRequest request) {
  mojo::MakeStrongBinding(
      std::make_unique<KeyboardLockServiceImpl>(render_frame_host),
      std::move(request));
}

void KeyboardLockServiceImpl::RequestKeyboardLock(
    const std::vector<std::string>& key_codes,
    RequestKeyboardLockCallback callback) {
  if (key_codes.empty()) {
    UMA_HISTOGRAM_ENUMERATION("KeyboardLock.ApiCalled", kRequestAllKeys,
                              kMaxValue);
  } else {
    UMA_HISTOGRAM_ENUMERATION("KeyboardLock.ApiCalled", kRequestSomeKeys,
                              kMaxValue);
  }

  // TODO(joedow): Implementation required.
  std::move(callback).Run(blink::mojom::KeyboardLockRequestResult::SUCCESS);
}

void KeyboardLockServiceImpl::CancelKeyboardLock() {
  UMA_HISTOGRAM_ENUMERATION("KeyboardLock.ApiCalled", kCancelLock, kMaxValue);

  // TODO(joedow): Implementation required.
}

}  // namespace content
