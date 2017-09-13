// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/keyboard_lock/keyboard_lock_service_impl.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "content/browser/keyboard_lock/host.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

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
        base::MakeUnique<KeyboardLockServiceImpl>(render_frame_host),
        std::move(request));
}

void KeyboardLockServiceImpl::RequestKeyboardLock(
    const std::vector<std::string>& key_codes,
    RequestKeyboardLockCallback callback) {
  keyboard_lock::Host::GetInstance()->Reserve(
      web_contents_,
      key_codes,
      base::Bind([](RequestKeyboardLockCallback&& callback, bool result) {
                   std::move(callback).Run(
                       blink::mojom::KeyboardLockRequestResult::SUCCESS);
                 },
                 base::Passed(std::move(callback))));
}

void KeyboardLockServiceImpl::CancelKeyboardLock() {
  keyboard_lock::Host::GetInstance()->ClearReserved(
      web_contents_,
      base::Callback<void(bool)>());
}

}  // namespace content
