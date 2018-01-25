// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/contextualsearch/unhandled_tap_notifier_impl.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "content/public/common/use_zoom_for_dsf_policy.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace contextual_search {

UnhandledTapNotifierImpl::UnhandledTapNotifierImpl(
    float device_scale_factor,
    UnhandledTapCallback callback)
    : device_scale_factor_(device_scale_factor),
      unhandled_tap_callback_(callback) {}

UnhandledTapNotifierImpl::~UnhandledTapNotifierImpl() {}

void UnhandledTapNotifierImpl::ShowUnhandledTapUIIfNeeded(
    blink::mojom::UnhandledTapInfoPtr unhandled_tap_info) {
  // Logic used to be in render_widget.cc ShowUnhandledTapUIIfNeeded.
  bool page_changed =
      unhandled_tap_info->dom_tree_changed || unhandled_tap_info->style_changed;
  bool should_trigger =
      !page_changed && unhandled_tap_info->is_text_node &&
      !unhandled_tap_info->is_content_editable &&
      !unhandled_tap_info->is_inside_focusable_element_or_aria_widget;
  if (should_trigger) {
    float x_px = unhandled_tap_info->tapped_position_in_viewport_x;
    float y_px = unhandled_tap_info->tapped_position_in_viewport_y;
    if (!content::UseZoomForDSFEnabled()) {
      x_px /= device_scale_factor_;
      y_px /= device_scale_factor_;
    }
    // The callback should always still be available, unless we're being killed.
    DCHECK(unhandled_tap_callback_);
    // Call back through the callback.
    unhandled_tap_callback_.Run(x_px, y_px);
  }
}

// static
void CreateUnhandledTapNotifierImpl(
    float device_scale_factor,
    UnhandledTapCallback callback,
    blink::mojom::UnhandledTapNotifierRequest request) {
  mojo::MakeStrongBinding(
      std::make_unique<UnhandledTapNotifierImpl>(device_scale_factor, callback),
      std::move(request));
}

}  // namespace contextual_search
