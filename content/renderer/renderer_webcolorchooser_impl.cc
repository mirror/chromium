// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/renderer_webcolorchooser_impl.h"

#include "content/public/renderer/render_frame.h"
#include "third_party/WebKit/common/associated_interfaces/associated_interface_provider.h"

namespace content {

static int GenerateColorChooserIdentifier() {
  static int next = 0;
  return ++next;
}

RendererWebColorChooserImpl::RendererWebColorChooserImpl(
    RenderFrame* render_frame,
    blink::WebColorChooserClient* client)
    : RenderFrameObserver(render_frame),
      identifier_(GenerateColorChooserIdentifier()),
      client_(client),
      client_binding_(this) {
  render_frame->GetRemoteAssociatedInterfaces()->GetInterface(&color_chooser_);
}

RendererWebColorChooserImpl::~RendererWebColorChooserImpl() {
}

void RendererWebColorChooserImpl::SetSelectedColor(blink::WebColor color) {
  color_chooser_->SetSelectedColorInColorChooser(identifier_, color);
}

void RendererWebColorChooserImpl::EndChooser() {
  color_chooser_->EndColorChooser(identifier_);
}

void RendererWebColorChooserImpl::Open(
    SkColor initial_color,
    std::vector<content::mojom::ColorSuggestionPtr> suggestions) {
  content::mojom::ColorChooserClientAssociatedPtrInfo client_ptr_info;
  client_binding_.Bind(mojo::MakeRequest(&client_ptr_info));
  color_chooser_->OpenColorChooser(std::move(client_ptr_info), identifier_,
                                   initial_color, std::move(suggestions));
}

void RendererWebColorChooserImpl::DidChooseColorResponse(
    uint32_t color_chooser_id,
    SkColor color) {
  DCHECK(identifier_ == color_chooser_id);

  client_->DidChooseColor(static_cast<blink::WebColor>(color));
}

void RendererWebColorChooserImpl::DidEndColorChooser(
    uint32_t color_chooser_id) {
  if (identifier_ != color_chooser_id)
    return;
  client_->DidEndChooser();
}

}  // namespace content
