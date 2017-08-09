// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/equivalence_map.h"

#include <stddef.h>

#include <algorithm>

#include "base/logging.h"
#include "chrome/installer/zucchini/encoded_view.h"
#include "chrome/installer/zucchini/suffix_array.h"

namespace zucchini {

double GetTokenSimilarity(const ImageIndex& old_image,
                          const ImageIndex& new_image,
                          offset_t src,
                          offset_t dst) {
  DCHECK(old_image.IsToken(src));
  DCHECK(new_image.IsToken(dst));

  TypeTag old_type = old_image.GetType(src);
  TypeTag new_type = new_image.GetType(dst);
  if (old_type != new_type)
    return kMismatchFatal;

  // Raw comparison.
  if (!old_image.IsReference(src) && !new_image.IsReference(dst))
    return old_image.GetRawValue(src) == new_image.GetRawValue(dst) ? 1.0
                                                                    : -1.5;

  DCHECK(old_type == new_type);  // Sanity check.
  Reference old_reference = old_image.FindReference(old_type, src);
  Reference new_reference = new_image.FindReference(new_type, dst);

  // Both targets are not associated, which implies a weak match.
  if (!IsMarked(old_reference.target) && !IsMarked(new_reference.target))
    return 0.5 * old_image.GetTraits(old_type).width;

  // At least one target is associated, so values are compared.
  return old_reference.target == new_reference.target
             ? old_image.GetTraits(old_type).width
             : -2.0;
}

double GetEquivalenceSimilarity(const ImageIndex& old_image,
                                const ImageIndex& new_image,
                                const Equivalence& equivalence) {
  double similarity = 0.0;
  for (offset_t k = 0; k < equivalence.length; ++k) {
    // Non-tokens are joined with the nearest previous token: skip until we
    // cover the unit, and extend |best_pos| if applicable.
    if (!new_image.IsToken(equivalence.dst_offset + k))
      continue;

    similarity +=
        GetTokenSimilarity(old_image, new_image, equivalence.src_offset + k,
                           equivalence.dst_offset + k);
    if (similarity == kMismatchFatal)
      return kMismatchFatal;
  }
  return similarity;
}

EquivalenceCandidate ExtendEquivalenceForward(const ImageIndex& old_image,
                                              const ImageIndex& new_image,
                                              EquivalenceCandidate equivalence,
                                              double minimum_similarity) {
  offset_t best_offset = equivalence.length;
  double current_similarity = equivalence.similarity;
  double current_penalty = minimum_similarity;
  for (offset_t k = best_offset;
       equivalence.src_offset + k < old_image.size() &&
       equivalence.dst_offset + k < new_image.size();
       ++k) {
    // Mismatch in type, |equivalence| can not be extended further.
    if (old_image.GetType(equivalence.src_offset + k) !=
        new_image.GetType(equivalence.dst_offset + k))
      break;

    if (!new_image.IsToken(equivalence.dst_offset + k)) {
      // Non-tokens are joined with the nearest previous token: skip until we
      // cover the unit, and extend |best_pos| if applicable.
      if (best_offset == k)
        best_offset = k + 1;
      continue;
    }

    DCHECK(old_image.GetType(equivalence.src_offset + k) ==
           new_image.GetType(equivalence.dst_offset + k));  // Sanity check.
    double similarity =
        GetTokenSimilarity(old_image, new_image, equivalence.src_offset + k,
                           equivalence.dst_offset + k);
    current_similarity += similarity;
    current_penalty = std::max(0.0, current_penalty) - similarity;

    if (current_similarity < 0.0 || current_penalty >= minimum_similarity)
      break;
    if (current_similarity >= equivalence.similarity) {
      equivalence.similarity = current_similarity;
      best_offset = k + 1;
    }
  }

  equivalence.length = best_offset;
  return equivalence;
}

EquivalenceCandidate ExtendEquivalenceBackward(const ImageIndex& old_image,
                                               const ImageIndex& new_image,
                                               EquivalenceCandidate equivalence,
                                               double minimum_similarity) {
  offset_t best_offset = 0;
  double current_similarity = equivalence.similarity;
  double current_penalty = 0.0;
  for (offset_t k = 1;
       k <= equivalence.dst_offset && k <= equivalence.src_offset; ++k) {
    // Mismatch in type, |equivalence| can not be extended further.
    if (old_image.GetType(equivalence.src_offset - k) !=
        new_image.GetType(equivalence.dst_offset - k))
      break;

    // Non-tokens are joined with the nearest previous token: skip until we
    // reach the next token.
    if (!new_image.IsToken(equivalence.dst_offset - k))
      continue;

    DCHECK(old_image.GetType(equivalence.src_offset - k) ==
           new_image.GetType(equivalence.dst_offset - k));  // Sanity check.
    double similarity =
        GetTokenSimilarity(old_image, new_image, equivalence.src_offset - k,
                           equivalence.dst_offset - k);
    current_similarity += similarity;
    current_penalty = std::max(0.0, current_penalty) - similarity;

    if (current_similarity < 0.0 || current_penalty >= minimum_similarity)
      break;
    if (current_similarity >= equivalence.similarity) {
      equivalence.similarity = current_similarity;
      best_offset = k;
    }
  }

  equivalence.dst_offset -= best_offset;
  equivalence.src_offset -= best_offset;
  equivalence.length += best_offset;
  return equivalence;
}

EquivalenceCandidate VisitEquivalenceSeed(const ImageIndex& old_view,
                                          const ImageIndex& new_view,
                                          offset_t src,
                                          offset_t dst,
                                          double minimum_similarity) {
  EquivalenceCandidate equivalence{src, dst, 0, 0.0};  // Empty.
  equivalence = ExtendEquivalenceForward(old_view, new_view, equivalence,
                                         minimum_similarity);
  if (equivalence.similarity < minimum_similarity)
    return equivalence;  // Not worth exploring any more.
  equivalence = ExtendEquivalenceBackward(old_view, new_view, equivalence,
                                          minimum_similarity);
  return equivalence;
}

EquivalenceMap::EquivalenceMap() = default;
EquivalenceMap::~EquivalenceMap() = default;

void EquivalenceMap::Build(const std::vector<offset_t>& old_sa,
                           const ImageIndex& old_image,
                           const ImageIndex& new_image,
                           double minimum_similarity) {
  DCHECK_EQ(old_sa.size(), old_image.size());

  FindCandidates(old_sa, old_image, new_image, minimum_similarity);

  // Sort by destination
  std::sort(equivalences_.begin(), equivalences_.end(),
            [](const EquivalenceCandidate& a, const EquivalenceCandidate& b) {
              return a.dst_offset < b.dst_offset;
            });

  Prune(old_image, new_image, minimum_similarity);

  offset_t coverage = 0;
  offset_t current_offset = 0;
  for (auto equivalence : equivalences_) {
    DCHECK_GE(equivalence.dst_offset, current_offset);
    coverage += equivalence.length;
    current_offset = equivalence.dst_offset + equivalence.length;
  }
  LOG(INFO) << "Equivalence Count: " << size();
  LOG(INFO) << "Coverage / Extra / Total: " << coverage << " / "
            << new_image.size() - coverage << " / " << new_image.size();
}

void EquivalenceMap::FindCandidates(const std::vector<offset_t>& old_sa,
                                    const ImageIndex& old_image,
                                    const ImageIndex& new_image,
                                    double minimum_similarity) {
  // We are building a suffix array of |old_view| to find seeds of Equivalences.
  equivalences_.clear();

  // This is an heuristic to find 'good' Equivalences on encoded views.
  // Equivalences are found in ascending order of |new_image|.
  EncodedView old_view(&old_image);
  EncodedView new_view(&new_image);
  offset_t dst_offset = 0;

  while (dst_offset < new_image.size()) {
    if (!new_image.IsToken(dst_offset)) {
      ++dst_offset;
      continue;
    }
    auto match =
        SuffixLowerBound(old_sa, old_view.begin(),
                         new_view.begin() + dst_offset, new_view.end());

    offset_t next_dst_offset = dst_offset + 1;
    double best_similarity = minimum_similarity;
    EquivalenceCandidate best_equivalence = {0, 0, 0, 0.0};
    for (auto it = match; it != old_sa.end(); ++it) {
      EquivalenceCandidate equivalence =
          VisitEquivalenceSeed(old_image, new_image, static_cast<offset_t>(*it),
                               dst_offset, minimum_similarity);
      if (equivalence.similarity > best_similarity) {
        best_equivalence = equivalence;
        best_similarity = equivalence.similarity;
        next_dst_offset = equivalence.dst_offset + equivalence.length;
      } else {
        break;
      }
    }
    for (auto it = match; it != old_sa.begin(); --it) {
      EquivalenceCandidate equivalence = VisitEquivalenceSeed(
          old_image, new_image, static_cast<offset_t>(it[-1]), dst_offset,
          minimum_similarity);
      if (equivalence.similarity > best_similarity) {
        best_equivalence = equivalence;
        best_similarity = equivalence.similarity;
        next_dst_offset = equivalence.dst_offset + equivalence.length;
      } else {
        break;
      }
    }
    if (best_equivalence.similarity >= minimum_similarity) {
      equivalences_.push_back(best_equivalence);
    }

    dst_offset = next_dst_offset;
  }
}

void EquivalenceMap::Prune(const ImageIndex& old_image,
                           const ImageIndex& new_image,
                           double minimum_similarity) {
  for (auto current = equivalences_.begin(); current != equivalences_.end();
       ++current) {
    if (current->similarity < minimum_similarity)
      continue;  // This equivalence will be discarded anyways.

    // Look ahead to resolve overlaps, until a better equivalence is found.
    for (auto next = current + 1; next != equivalences_.end(); ++next) {
      DCHECK_GE(next->dst_offset, current->dst_offset);
      if (next->dst_offset >= current->dst_offset + current->length)
        break;  // No more overlap.

      offset_t overlap =
          current->dst_offset + current->length - next->dst_offset;

      // |next| is better, so |current| shrinks.
      if (current->similarity < next->similarity) {
        current->length -= overlap;
        current->similarity = GetEquivalenceSimilarity(
            old_image, new_image,
            Equivalence{current->src_offset, current->dst_offset,
                        current->length});
        break;
      }
    }

    // Shrinks all overlapping equivalences following and worse than |current|.
    for (auto next = current + 1; next != equivalences_.end(); ++next) {
      if (next->dst_offset >= current->dst_offset + current->length)
        break;  // No more overlap.

      offset_t overlap =
          current->dst_offset + current->length - next->dst_offset;
      next->length = next->length > overlap ? next->length - overlap : 0;
      next->src_offset += overlap;
      next->dst_offset += overlap;
      next->similarity = GetEquivalenceSimilarity(
          old_image, new_image,
          {next->src_offset, next->dst_offset, next->length});
      DCHECK(next->dst_offset == current->dst_offset + current->length);
    }
  }

  // Discard all equivalences with similarity smaller than |minimum_similarity|.
  equivalences_.erase(
      std::remove_if(
          equivalences_.begin(), equivalences_.end(),
          [minimum_similarity](const EquivalenceCandidate& equivalence) {
            return equivalence.similarity < minimum_similarity;
          }),
      equivalences_.end());
}

}  // namespace zucchini
