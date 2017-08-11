// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/dom_distiller/content/renderer/distillability_webagent.h"

#include "components/dom_distiller/content/common/distillability_service.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/WebKit/webagents/document.h"
#include "third_party/WebKit/webagents/element.h"

namespace dom_distiller {

using namespace webagents;

DistillabilityWebagent::DistillabilityWebagent(
    content::RenderFrame* render_frame)
    : RenderFrameObserver(render_frame),
      webagents::Agent(render_frame->GetWebFrame()) {
  LOG(ERROR) << "DistillabilityWebagent=" << this;
}

void DistillabilityWebagent::DidMeaningfulLayout(
    blink::WebMeaningfulLayout layout_type) {
  if (layout_type != blink::WebMeaningfulLayout::kFinishedParsing &&
      layout_type != blink::WebMeaningfulLayout::kFinishedLoading) {
    return;
  }

  DCHECK(GetFrame());
  // if (!GetFrame()->IsMainFrame()) return;

  LOG(ERROR) << "DistillabilityWebagent::DidMeaningfulLayout=" << this
             << ", layout_type=" << layout_type;

  Document* doc = GetFrame()->GetDocument();
  // if (doc.IsNull() || doc.Body().IsNull())
  if (!doc)
    return;
  // if (!url_utils::IsUrlDistillable(doc.Url()))
  //  return;

  bool is_loaded = layout_type == blink::WebMeaningfulLayout::kFinishedLoading;
  // if (!NeedToUpdate(is_loaded)) return;
  if (!is_loaded)
    return;

  // bool is_last = IsLast(is_loaded);
  bool is_last = is_loaded;
  // bool is_distillable_page = IsDistillablePage(doc, is_last);
  bool is_distillable_page = false;

  // Connect to Mojo service on browser to notify page distillability.
  mojom::DistillabilityServicePtr distillability_service;
  GetFrame()->GetInterfaceProvider().GetInterface(&distillability_service);
  DCHECK(distillability_service);
  if (!distillability_service.is_bound())
    return;
  LOG(ERROR) << "DomDistillerWebagent calling mojo";
  distillability_service->NotifyIsDistillable(is_distillable_page, is_last);
}

void DistillabilityWebagent::OnDestruct() {
  delete this;
}

}  // namespace dom_distiller
