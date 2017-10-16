// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/csspaint/PaintWorklet.h"

#include "bindings/core/v8/V8BindingForCore.h"
#include "core/dom/Document.h"
#include "core/frame/LocalDOMWindow.h"
#include "core/frame/LocalFrame.h"
#include "modules/csspaint/CSSPaintDefinition.h"
#include "modules/csspaint/PaintWorkletGlobalScope.h"
#include "platform/graphics/Image.h"
#include "platform/wtf/CryptographicallyRandomNumber.h"

namespace blink {

const size_t PaintWorklet::kNumGlobalScopes = 2u;
const size_t kMaxPaintCountToSwitch = 30u;
DocumentPaintDefinition* const kInvalidDocumentDefinition = nullptr;

// static
PaintWorklet* PaintWorklet::From(LocalDOMWindow& window) {
  PaintWorklet* supplement = static_cast<PaintWorklet*>(
      Supplement<LocalDOMWindow>::From(window, SupplementName()));
  if (!supplement && window.GetFrame()) {
    supplement = Create(window.GetFrame());
    ProvideTo(window, SupplementName(), supplement);
  }
  return supplement;
}

// static
PaintWorklet* PaintWorklet::Create(LocalFrame* frame) {
  return new PaintWorklet(frame);
}

PaintWorklet::PaintWorklet(LocalFrame* frame)
    : Worklet(frame),
      Supplement<LocalDOMWindow>(*frame->DomWindow()),
      pending_generator_registry_(new PaintWorkletPendingGeneratorRegistry) {}

PaintWorklet::~PaintWorklet() = default;

void PaintWorklet::AddPendingGenerator(const String& name,
                                       CSSPaintImageGeneratorImpl* generator) {
  pending_generator_registry_->AddPendingGenerator(name, generator);
}

// We start with a random global scope when a new frame starts. Then within this
// frame, we switch to the other global scope after certain amount of paint
// calls (rand(kMaxPaintCountToSwitch)).
// This approach ensures non-deterministic of global scope selecting, and that
// there is a max of one switching within one frame.
size_t PaintWorklet::SelectGlobalScope() {
  size_t current_paint_frame_count = GetFrame()->View()->PaintFrameCount();
  size_t selected_global_scope = active_global_scope_;
  // Whether a new frame starts or not.
  bool frame_changed = current_paint_frame_count != active_frame_count_;
  if (frame_changed) {
    SetPaintsBeforeSwitching();
    active_frame_count_ = current_paint_frame_count;
  }
  // We switch when |paints_before_switching_global_scope_| is 1 instead of 0
  // because the var keeps decrementing and stays at 0.
  if (frame_changed || paints_before_switching_global_scope_ == 1) {
    selected_global_scope = CryptographicallyRandomNumber() % kNumGlobalScopes;
    active_global_scope_ = selected_global_scope;
  }
  if (paints_before_switching_global_scope_ > 0)
    paints_before_switching_global_scope_--;
  return selected_global_scope;
}

void PaintWorklet::SetPaintsBeforeSwitching() {
  // TODO(xidachen): Try to make this variable re-usable for the next frame.
  // If one frame typically has ~5 paint, then we can switch after a few
  // frames.
  paints_before_switching_global_scope_ =
      CryptographicallyRandomNumber() % kMaxPaintCountToSwitch;
}

RefPtr<Image> PaintWorklet::Paint(const String& name,
                                  const ImageResourceObserver& observer,
                                  const IntSize& container_size,
                                  const CSSStyleValueVector* data,
                                  const LayoutSize* logical_size) {
  if (!document_definition_map_.Contains(name))
    return nullptr;

  // Check if the existing document definition is valid or not.
  DocumentPaintDefinition* document_definition =
      document_definition_map_.at(name);
  if (document_definition == kInvalidDocumentDefinition)
    return nullptr;

  PaintWorkletGlobalScopeProxy* proxy =
      PaintWorkletGlobalScopeProxy::From(FindAvailableGlobalScope());
  CSSPaintDefinition* paint_definition = proxy->FindDefinition(name);
  return paint_definition->Paint(observer, container_size, data, logical_size);
}

const char* PaintWorklet::SupplementName() {
  return "PaintWorklet";
}

DEFINE_TRACE(PaintWorklet) {
  visitor->Trace(pending_generator_registry_);
  visitor->Trace(document_definition_map_);
  Worklet::Trace(visitor);
  Supplement<LocalDOMWindow>::Trace(visitor);
}

bool PaintWorklet::NeedsToCreateGlobalScope() {
  return GetNumberOfGlobalScopes() < kNumGlobalScopes;
}

WorkletGlobalScopeProxy* PaintWorklet::CreateGlobalScope() {
  DCHECK(NeedsToCreateGlobalScope());
  return new PaintWorkletGlobalScopeProxy(
      ToDocument(GetExecutionContext())->GetFrame(),
      pending_generator_registry_, GetNumberOfGlobalScopes() + 1);
}

}  // namespace blink
