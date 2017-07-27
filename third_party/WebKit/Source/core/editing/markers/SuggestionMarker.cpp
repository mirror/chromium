// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/markers/SuggestionMarker.h"

namespace blink {

SuggestionMarker::SuggestionMarker(
    unsigned start_offset,
    unsigned end_offset,
    int32_t tag,
    const Vector<String>& suggestions,
    RemoveUponReplace remove_upon_suggestion_replacement,
    Color suggestion_highlight_color,
    Color underline_color,
    Thickness thickness,
    Color background_color)
    : StyleableMarker(start_offset,
                      end_offset,
                      underline_color,
                      thickness,
                      background_color),
      tag_(tag),
      suggestions_(suggestions),
      remove_upon_suggestion_replacement_(remove_upon_suggestion_replacement),
      suggestion_highlight_color_(suggestion_highlight_color) {}

int32_t SuggestionMarker::Tag() const {
  return tag_;
}

DocumentMarker::MarkerType SuggestionMarker::GetType() const {
  return DocumentMarker::kSuggestion;
}

const Vector<String>& SuggestionMarker::Suggestions() const {
  return suggestions_;
}

bool SuggestionMarker::ShouldRemoveUponSuggestionReplacement() const {
  return remove_upon_suggestion_replacement_ == RemoveUponReplace::kRemove;
}

Color SuggestionMarker::SuggestionHighlightColor() const {
  return suggestion_highlight_color_;
}

void SuggestionMarker::ReplaceSuggestion(int suggestion_index,
                                         const String& new_suggestion) {
  DCHECK_LT(suggestion_index, static_cast<int>(suggestions_.size()));
  suggestions_[suggestion_index] = new_suggestion;
}

}  // namespace blink
