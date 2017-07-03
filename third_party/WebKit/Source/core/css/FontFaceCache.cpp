/*
 * Copyright (C) 2007, 2008, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "core/css/FontFaceCache.h"

#include "core/css/CSSSegmentedFontFace.h"
#include "core/css/CSSValueList.h"
#include "core/css/FontFace.h"
// #include "core/css/FontStyleMatcher.h"
#include "core/css/StyleRule.h"
#include "core/loader/resource/FontResource.h"
#include "platform/FontFamilyNames.h"
#include "platform/fonts/FontDescription.h"
#include "platform/fonts/FontSelectionAlgorithm.h"
#include "platform/loader/fetch/ResourceFetcher.h"
#include "platform/wtf/text/AtomicString.h"

namespace blink {

static unsigned g_version = 0;

FontFaceCache::FontFaceCache() : version_(0) {}

void FontFaceCache::Add(const StyleRuleFontFace* font_face_rule,
                        FontFace* font_face) {
  if (!style_rule_to_font_face_.insert(font_face_rule, font_face).is_new_entry)
    return;
  AddFontFace(font_face, true);
}

/** This method receives a new font face instantiated from a @font-face
 * descriptor, and adds it to the internal lookup structures. The first grouping
 * category is the family name. For each family name, next a lookup is performed
 * by font traits, which consiste of stretch, style/slope, weight. The result of
 * this lookup is a CSSSegmentedFontFace, which is the set of CSSFontFaces with
 * identical family, and identical traits, but different unicode-ranges.
 *
 * In the new world, where @font-faces can have FontSelectionCapabilities,
 * instead of Traits, we cannot really lookup hashed by traits anymore, but
 * perhaps we need to keep a full vector around only grouped by family. Once we
 * get to the stage of retrieving a CSSSegmentedFontFace, we get the Vector for
 * a family, then filter by Traits (and later a FontSelectionRequest) and
 * compile a new CSSSegmentedFontFace with all faces that are a match/filtered
 * by the requested Traits.

 * In the transition phase of splitting Traits and FontSelectionCapabilities
 * since the capabilities do not yet have ranges, a direct hash table lookup
 * from Traits is still possible. This would be done by creating a useful hash
 * function from FontSelectionCapabilities, then converting a Traits object to a
 * FontSelectionCapabilities object and performing a lookup into the available
 * CSSSegmentedFontFace in TraitsMap. However, this would be temporary, as
 * later, a hash lookup is not possible anymore since we do need the in-range
 * checks, which cannot be performed by computing hashes.

 * The lookups themselves, later including the range checks, should be optimised
 * for the incoming FontSelectionRequests. The result of the
 * FontSelectionAlgorithm can then be cached in a
 * HashMap<FontSelectionRequest,RefPtr<CSSSegmentedFontFace>>.

 * As a result, the hashing in the add methods will go away, as there is no
 * possible precomputation of matching results. Previously, hashing in these
 * functions was optimizing the lookup, but not any more since we have to match
 * ranges.
 */
void FontFaceCache::AddFontFace(FontFace* font_face, bool css_connected) {
  SegmentedFacesByFamily::AddResult capabilities_result =
      segmented_faces_.insert(font_face->family(), nullptr);

  if (capabilities_result.is_new_entry)
    capabilities_result.stored_value->value = new CapabilitiesSet();

  DCHECK(font_face->GetFontSelectionCapabilities().IsValid() &&
         !font_face->GetFontSelectionCapabilities().IsHashTableDeletedValue());

  CapabilitiesSet::AddResult segmented_font_face_result =
      capabilities_result.stored_value->value->insert(
          font_face->GetFontSelectionCapabilities(), nullptr);
  if (segmented_font_face_result.is_new_entry) {
    segmented_font_face_result.stored_value->value =
        CSSSegmentedFontFace::Create(font_face->GetFontSelectionCapabilities());
  }

  segmented_font_face_result.stored_value->value->AddFontFace(font_face,
                                                              css_connected);

  if (css_connected)
    css_connected_font_faces_.insert(font_face);

  font_selection_query_cache_.erase(font_face->family());
  IncrementVersion();
}

void FontFaceCache::Remove(const StyleRuleFontFace* font_face_rule) {
  StyleRuleToFontFace::iterator it =
      style_rule_to_font_face_.find(font_face_rule);
  if (it != style_rule_to_font_face_.end()) {
    RemoveFontFace(it->value.Get(), true);
    style_rule_to_font_face_.erase(it);
  }
}

void FontFaceCache::RemoveFontFace(FontFace* font_face, bool css_connected) {
  SegmentedFacesByFamily::iterator segmented_faces_iter =
      segmented_faces_.find(font_face->family());
  if (segmented_faces_iter == segmented_faces_.end())
    return;

  CapabilitiesSet* family_segmented_faces = segmented_faces_iter->value.Get();

  CapabilitiesSet::iterator family_segmented_faces_iter =
      family_segmented_faces->find(font_face->GetFontSelectionCapabilities());
  if (family_segmented_faces_iter == family_segmented_faces->end())
    return;

  CSSSegmentedFontFace* segmented_font_face =
      family_segmented_faces_iter->value;
  segmented_font_face->RemoveFontFace(font_face);
  if (segmented_font_face->IsEmpty()) {
    family_segmented_faces->erase(family_segmented_faces_iter);
    if (family_segmented_faces->IsEmpty()) {
      segmented_faces_.erase(segmented_faces_iter);
    }
  }

  font_selection_query_cache_.erase(font_face->family());

  if (css_connected)
    css_connected_font_faces_.erase(font_face);

  IncrementVersion();
}

void FontFaceCache::ClearCSSConnected() {
  for (const auto& item : style_rule_to_font_face_)
    RemoveFontFace(item.value.Get(), true);
  style_rule_to_font_face_.clear();
}

void FontFaceCache::ClearAll() {
  if (segmented_faces_.IsEmpty())
    return;

  segmented_faces_.clear();
  font_selection_query_cache_.clear();
  style_rule_to_font_face_.clear();
  css_connected_font_faces_.clear();
  IncrementVersion();
}

void FontFaceCache::IncrementVersion() {
  version_ = ++g_version;
}

std::ostream& operator<<(std::ostream& os,
                         const FontSelectionRequest& request) {
  os << "Stretch: " << static_cast<float>(request.width)
     << " Style: " << static_cast<float>(request.slope)
     << " Weight: " << static_cast<float>(request.weight);
  return os;
}

CSSSegmentedFontFace* FontFaceCache::Get(
    const FontDescription& font_description,
    const AtomicString& family) {
  SegmentedFacesByFamily::iterator segmented_faces_for_family =
      segmented_faces_.find(family);
  if (segmented_faces_for_family == segmented_faces_.end() ||
      segmented_faces_for_family->value->IsEmpty())
    return nullptr;

  auto family_faces = segmented_faces_for_family->value;

  // Either add or retrieve a cache entry in the selection query cache for the
  // specified family.
  FontSelectionQueryCache::AddResult cache_entry_for_family_add =
      font_selection_query_cache_.insert(family,
                                         new FontSelectionQueryResult());
  auto cache_entry_for_family = cache_entry_for_family_add.stored_value->value;

  const FontSelectionRequest& request =
      font_description.GetFontSelectionRequest();

  LOG(INFO) << request;

  FontSelectionQueryResult::AddResult face_entry =
      cache_entry_for_family->insert(request, nullptr);
  if (!face_entry.is_new_entry)
    return face_entry.stored_value->value;

  // If we don't have a previously cached result for this request, we now need
  // to iterate over all entries in the CapabilitiesSet for one family and
  // extract the best CSSSegmentedFontFace from those.

  // The FontSelectionAlgorithm needs to know the boundaries of stretch, style,
  // range for all the available faces in order to calculate distances
  // correctly.
  FontSelectionCapabilities all_faces_boundaries;
  for (const auto& item : *family_faces) {
    all_faces_boundaries.Expand(item.value->GetFontSelectionCapabilities());
  }

  FontSelectionAlgorithm font_selection_algorithm(request,
                                                  all_faces_boundaries);
  for (const auto& item : *family_faces) {
    // TODO: Here, compare the capabilities of the CSSSegmentedFontFace against
    // previous capabilities, and assign the face entry.
    const FontSelectionCapabilities& candidate_key = item.key;
    CSSSegmentedFontFace* candidate_value = item.value;
    if (!face_entry.stored_value->value ||
        font_selection_algorithm.IsBetterMatchForRequest(
            candidate_key,
            face_entry.stored_value->value->GetFontSelectionCapabilities())) {
      // TODO: We need to keep information about how the match was produced,
      // i.e. somehow store either the perfect "in-range" map information, or
      // store, at which boundaries values the match for stretch, style, weight
      // was made.
      // https://drafts.csswg.org/css-fonts-4/#apply-font-matching-variations
      // Values must be clamped to the result of the font matching algorithm,
      // then by the boundaries of the actual font.

      // It should be possible to apply the ideal clamped values to the
      // CSSSegmentedFontFace since these are returned individually for each
      // incoming FontSelectionRequest (at this point built from Traits).
      face_entry.stored_value->value = candidate_value;
    }
  }
  return face_entry.stored_value->value;
}

size_t FontFaceCache::GetNumSegmentedFacesForTesting() {
  size_t count = 0;
  for (auto& family_faces : segmented_faces_) {
    count += family_faces.value->size();
  }
  return count;
}

DEFINE_TRACE(FontFaceCache) {
  visitor->Trace(segmented_faces_);
  visitor->Trace(font_selection_query_cache_);
  visitor->Trace(style_rule_to_font_face_);
  visitor->Trace(css_connected_font_faces_);
}

}  // namespace blink
