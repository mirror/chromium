// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/media_capabilities/MediaCapabilitiesController.h"

#include "core/dom/Document.h"
#include "core/frame/LocalFrame.h"

namespace blink {

// static
void MediaCapabilitiesController::ProvideTo(
    LocalFrame& frame,
    WebMediaCapabilitiesClient* client) {
  Supplement<LocalFrame>::ProvideTo(
      frame, SupplementName(), new MediaCapabilitiesController(frame, client));
}

// static
const char* MediaCapabilitiesController::SupplementName() {
  return "MediaCapabilitiesController";
}

// static
MediaCapabilitiesController* MediaCapabilitiesController::From(
    LocalFrame& frame) {
  return static_cast<MediaCapabilitiesController*>(
      Supplement<LocalFrame>::From(frame, SupplementName()));
}

void MediaCapabilitiesController::ContextDestroyed(ExecutionContext*) {
  client_ = nullptr;
}

WebMediaCapabilitiesClient* MediaCapabilitiesController::Client() {
  return client_;
}

MediaCapabilitiesController::MediaCapabilitiesController(
    LocalFrame& frame,
    WebMediaCapabilitiesClient* client)
    : ContextLifecycleObserver(frame.GetDocument()), client_(client) {}

DEFINE_TRACE(MediaCapabilitiesController) {
  Supplement<LocalFrame>::Trace(visitor);
  ContextLifecycleObserver::Trace(visitor);
}

}  // namespace blink
