// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/keyboard_lock/keyboard_lock_service_impl.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/memory/ptr_util.h"
#include "build/build_config.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "ui/aura/window.h"
#include "ui/events/keycodes/dom/keycode_converter.h"

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
  std::vector<int> native_codes;
  for (const std::string& code : key_codes) {
    native_codes.push_back(
        ui::KeycodeConverter::CodeStringToNativeKeycode(code));
  }
#if defined(USE_AURA)
  web_contents_->GetContentNativeView()->LockKeys(native_codes);
#elif defined(OS_MACOSX)
  // web_contents_->GetRenderWidgetHostView()->LockKeys(native_codes);
#endif
  std::move(callback).Run(blink::mojom::KeyboardLockRequestResult::SUCCESS);
}

void KeyboardLockServiceImpl::CancelKeyboardLock() {
#if defined(USE_AURA)
  if (web_contents_->GetContentNativeView()) {
    web_contents_->GetContentNativeView()->UnlockKeys();
  }
#elif defined(OS_MACOSX)
  // web_contents_->GetRenderWidgetHostView()->UnlockKeys();
#endif
}

}  // namespace content
