// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SuggestionMarker_h
#define SuggestionMarker_h

#include "core/editing/markers/StyleableMarker.h"

namespace blink {

// A subclass of StyleableMarker used to store information specific to
// suggestion markers (used to represent Android SuggestionSpans). In addition
// to the formatting information StyleableMarker holds, we also store a list of
// suggested replacements for the marked region of text. In addition, each
// SuggestionMarker is tagged with an integer so browser code can identify which
// SuggestionMarker a suggestion replace operation pertains to.
class CORE_EXPORT SuggestionMarker final : public StyleableMarker {
 public:
  enum class RemoveUponReplace { kDontRemove, kRemove };

  SuggestionMarker(unsigned start_offset,
                   unsigned end_offset,
                   int32_t tag,
                   const Vector<String>& suggestions,
                   RemoveUponReplace remove_upon_suggestion_replacement,
                   Color suggestion_highlight_color,
                   Color underline_color,
                   Thickness,
                   Color background_color);

  // DocumentMarker implementations
  MarkerType GetType() const final;

  // SuggestionMarker-specific
  int32_t Tag() const;
  const Vector<String>& Suggestions() const;
  bool ShouldRemoveUponSuggestionReplacement() const;
  Color SuggestionHighlightColor() const;

  // Replace the suggestion at suggestion_index with new_suggestion.
  void ReplaceSuggestion(int suggestion_index, const String& new_suggestion);

 private:
  int32_t tag_;
  Vector<String> suggestions_;
  RemoveUponReplace remove_upon_suggestion_replacement_;
  Color suggestion_highlight_color_;

  DISALLOW_COPY_AND_ASSIGN(SuggestionMarker);
};

DEFINE_TYPE_CASTS(SuggestionMarker,
                  DocumentMarker,
                  marker,
                  marker->GetType() == DocumentMarker::kSuggestion,
                  marker.GetType() == DocumentMarker::kSuggestion);

}  // namespace blink

#endif
