// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/markers/SuggestionMarkerReplacementScope.h"

#include "core/editing/markers/DocumentMarkerController.h"

namespace blink {

SuggestionMarkerReplacementScope::SuggestionMarkerReplacementScope(
    DocumentMarkerController* document_marker_controller)
    : document_marker_controller_(document_marker_controller) {
  document_marker_controller_->PrepareForSuggestionMarkerReplacement();
}

SuggestionMarkerReplacementScope::~SuggestionMarkerReplacementScope() {
  document_marker_controller_->SuggestionMarkerReplacementFinished();
}

}  // namespace blink
