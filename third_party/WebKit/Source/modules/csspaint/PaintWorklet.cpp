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
PaintWorklet* PaintWorklet::From(LocalDOMWindow& window,
                                 bool is_constructed_from_testing) {
  PaintWorklet* supplement = static_cast<PaintWorklet*>(
      Supplement<LocalDOMWindow>::From(window, SupplementName()));
  if (!supplement && window.GetFrame()) {
    supplement = Create(window.GetFrame(), is_constructed_from_testing);
    ProvideTo(window, SupplementName(), supplement);
  }
  return supplement;
}

// static
PaintWorklet* PaintWorklet::Create(LocalFrame* frame,
                                   bool is_constructed_from_testing) {
  return new PaintWorklet(frame, is_constructed_from_testing);
}

PaintWorklet::PaintWorklet(LocalFrame* frame, bool is_constructed_from_testing)
    : Worklet(frame),
      Supplement<LocalDOMWindow>(*frame->DomWindow()),
      pending_generator_registry_(new PaintWorkletPendingGeneratorRegistry),
      is_constructed_from_testing_(is_constructed_from_testing) {}

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
  size_t selected_global_scope = previously_selected_global_scope_;
  // A new frame starts.
  if (current_paint_frame_count != previous_paint_frame_cnt_) {
    // TODO(xidachen): Try to make this variable re-usable for the next frame.
    // If one frame typically has ~5 paint, then we can switch after a few
    // frames.
    if (is_constructed_from_testing_) {
      // In unit testing, we make it switch every 25-30 paint calls so that we
      // can have a stronger assertion in our testing.
      const size_t paint_count_to_switch_for_testing = 5;
      paints_before_switching_global_scope_ =
          CryptographicallyRandomNumber() % paint_count_to_switch_for_testing +
          25;
    } else {
      paints_before_switching_global_scope_ =
          CryptographicallyRandomNumber() % kMaxPaintCountToSwitch;
    }
    selected_global_scope = CryptographicallyRandomNumber() % kNumGlobalScopes;
    previous_paint_frame_cnt_ = current_paint_frame_count;
    previously_selected_global_scope_ = selected_global_scope;
  } else {
    if (paints_before_switching_global_scope_ == 0) {
      selected_global_scope =
          CryptographicallyRandomNumber() % kNumGlobalScopes;
      previously_selected_global_scope_ = selected_global_scope;
    }
  }
  paints_before_switching_global_scope_--;
  return selected_global_scope;
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
