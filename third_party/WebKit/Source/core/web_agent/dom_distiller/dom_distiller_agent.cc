// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/web_agent/dom_distiller/dom_distiller_agent.h"
#include "components/dom_distiller/content/common/distillability_service.mojom.h"
#include "core/web_agent/api/document.h"
#include "core/web_agent/api/element.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace web {

using namespace blink;
using namespace dom_distiller;

DomDistillerAgent::DomDistillerAgent(Frame* frame) : Agent(frame) {
  LOG(ERROR) << "DomDistillerAgent=" << this;
  Document* document = frame->GetDocument();
  if (!document)
    return;
  const Element* element = document->documentElement();
  if (!element)
    return;
}

void DomDistillerAgent::DidMeaningfulLayout(
    blink::WebMeaningfulLayout layout_type) {
  if (layout_type != WebMeaningfulLayout::kFinishedParsing &&
      layout_type != WebMeaningfulLayout::kFinishedLoading) {
    return;
  }

  DCHECK(GetFrame());
  // if (!GetFrame()->IsMainFrame()) return;

  LOG(ERROR) << "DomDistillerAgent::DidMeaningfulLayout=" << this
             << ", layout_type=" << layout_type;

  Document* doc = GetFrame()->GetDocument();
  // if (doc.IsNull() || doc.Body().IsNull())
  if (!doc)
    return;
  // if (!url_utils::IsUrlDistillable(doc.Url()))
  //  return;

  bool is_loaded = layout_type == WebMeaningfulLayout::kFinishedLoading;
  // if (!NeedToUpdate(is_loaded)) return;
  if (!is_loaded)
    return;

  // bool is_last = IsLast(is_loaded);
  bool is_last = is_loaded;
  // bool is_distillable_page = IsDistillablePage(doc, is_last);
  bool is_distillable_page = true;

  // Connect to Mojo service on browser to notify page distillability.
  mojom::DistillabilityServicePtr distillability_service;
  GetFrame()->GetInterfaceProvider().GetInterface(&distillability_service);
  DCHECK(distillability_service);
  if (!distillability_service.is_bound())
    return;
  distillability_service->NotifyIsDistillable(is_distillable_page, is_last);
}

}  // namespace web
