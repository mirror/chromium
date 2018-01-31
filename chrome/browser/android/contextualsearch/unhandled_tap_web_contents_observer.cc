// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/contextualsearch/unhandled_tap_web_contents_observer.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "chrome/browser/android/contextualsearch/unhandled_tap_notifier_impl.h"

namespace contextual_search {

UnhandledTapWebContentsObserver::UnhandledTapWebContentsObserver(
    content::WebContents* web_contents,
    float device_scale_factor,
    UnhandledTapCallback callback)
    : content::WebContentsObserver(web_contents) {
  registry_.AddInterface(
      base::BindRepeating(&contextual_search::CreateUnhandledTapNotifierImpl,
                          device_scale_factor, callback));
}

UnhandledTapWebContentsObserver::~UnhandledTapWebContentsObserver() {}

void UnhandledTapWebContentsObserver::OnInterfaceRequestFromFrame(
    content::RenderFrameHost* render_frame_host,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle* interface_pipe) {
  registry_.TryBindInterface(interface_name, interface_pipe);
}

}  // namespace contextual_search
