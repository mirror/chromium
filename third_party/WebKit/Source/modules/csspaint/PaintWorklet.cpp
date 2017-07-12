// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/csspaint/PaintWorklet.h"

#include "bindings/core/v8/V8BindingForCore.h"
#include "core/dom/Document.h"
#include "core/frame/LocalFrame.h"
#include "modules/csspaint/CSSPaintDefinition.h"
#include "modules/csspaint/PaintWorkletGlobalScope.h"
#include "platform/graphics/Image.h"

namespace blink {

// static
PaintWorklet* PaintWorklet::Create(LocalFrame* frame) {
  return new PaintWorklet(frame);
}

PaintWorklet::PaintWorklet(LocalFrame* frame)
    : Worklet(frame),
      pending_generator_registry_(new PaintWorkletPendingGeneratorRegistry) {}

PaintWorklet::~PaintWorklet() = default;

CSSPaintDefinition* PaintWorklet::FindDefinition(const String& name) {
  if (GetNumberOfGlobalScopes() == 0)
    return nullptr;

  PaintWorkletGlobalScopeProxy* proxy =
      PaintWorkletGlobalScopeProxy::From(FindAvailableGlobalScope());
  return proxy->FindDefinition(name);
}

void PaintWorklet::AddPendingGenerator(const String& name,
                                       CSSPaintImageGeneratorImpl* generator) {
  pending_generator_registry_->AddPendingGenerator(name, generator);
}

RefPtr<Image> PaintWorklet::Paint(const String& name,
                                  const ImageResourceObserver& observer,
                                  const IntSize& size,
                                  const CSSStyleValueVector* data) {
  if (!document_definition_map_.Contains(name))
    return nullptr;

  DocumentPaintDefinition* document_definition =
      document_definition_map_.at(name);
  if (!document_definition)
    return nullptr;

  return FindDefinition(name)->Paint(observer, size, data);
}

DEFINE_TRACE(PaintWorklet) {
  visitor->Trace(pending_generator_registry_);
  visitor->Trace(document_definition_map_);
  Worklet::Trace(visitor);
}

bool PaintWorklet::NeedsToCreateGlobalScope() {
  return GetNumberOfGlobalScopes() <= 1;
}

WorkletGlobalScopeProxy* PaintWorklet::CreateGlobalScope() {
  return new PaintWorkletGlobalScopeProxy(
      ToDocument(GetExecutionContext())->GetFrame(),
      pending_generator_registry_);
}

}  // namespace blink
