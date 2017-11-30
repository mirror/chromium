// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_RENDERER_WEBCOLORCHOOSER_IMPL_H_
#define CONTENT_RENDERER_RENDERER_WEBCOLORCHOOSER_IMPL_H_

#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/common/color_chooser.mojom.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "third_party/WebKit/public/web/WebColorChooser.h"
#include "third_party/WebKit/public/web/WebColorChooserClient.h"
#include "third_party/skia/include/core/SkColor.h"

namespace content {

class RendererWebColorChooserImpl : public blink::WebColorChooser,
                                    public content::mojom::ColorChooserClient,
                                    public RenderFrameObserver {
 public:
  explicit RendererWebColorChooserImpl(RenderFrame* render_frame,
                                       blink::WebColorChooserClient*);
  ~RendererWebColorChooserImpl() override;

  // blink::WebColorChooser implementation:
  void SetSelectedColor(const blink::WebColor) override;
  void EndChooser() override;

  void Open(SkColor initial_color,
            std::vector<content::mojom::ColorSuggestionPtr> suggestions);

  blink::WebColorChooserClient* client() { return client_; }

 private:
  // RenderFrameObserver implementation:
  // Don't destroy the RendererWebColorChooserImpl when the RenderFrame goes
  // away. RendererWebColorChooserImpl is owned by
  // blink::ColorChooserUIController.
  void OnDestruct() override {}

  // content::mojom::ColorChooserClient
  void DidChooseColorResponse(uint32_t color_chooser_id,
                              SkColor color) override;
  void DidEndColorChooser(uint32_t color_chooser_id) override;

  uint32_t identifier_;
  blink::WebColorChooserClient* client_;
  content::mojom::ColorChooserAssociatedPtr color_chooser_;
  mojo::AssociatedBinding<content::mojom::ColorChooserClient> client_binding_;

  DISALLOW_COPY_AND_ASSIGN(RendererWebColorChooserImpl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_RENDERER_WEBCOLORCHOOSER_IMPL_H_
