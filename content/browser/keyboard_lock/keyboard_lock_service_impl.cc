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
#include "content/browser/keyboard_lock/keyboard_lock_host.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"

namespace content {

KeyboardLockServiceImpl::KeyboardLockServiceImpl(
    RenderFrameHost* render_frame_host)
    : render_frame_host_(render_frame_host) {
  DCHECK(render_frame_host_);
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
  // RenderWidgetHostView may change over time so don't store it.
  RenderWidgetHostView* render_widget_host_view = render_frame_host_->GetView();
  if (render_widget_host_view) {
    render_widget_host_view->ReserveKeys(
        key_codes, base::BindOnce(
                       [](RequestKeyboardLockCallback&& callback, bool result) {
                         std::move(callback).Run(
                             blink::mojom::KeyboardLockRequestResult::SUCCESS);
                       },
                       base::Passed(std::move(callback))));
  } else {
    // TODO(joedow): Return an error here.
    std::move(callback).Run(blink::mojom::KeyboardLockRequestResult::SUCCESS);
  }
}

void KeyboardLockServiceImpl::CancelKeyboardLock() {
  // RenderWidgetHostView may change over time so don't store it.
  RenderWidgetHostView* render_widget_host_view = render_frame_host_->GetView();
  if (render_widget_host_view) {
    render_widget_host_view->ClearReservedKeys();
  }
}

}  // namespace content
