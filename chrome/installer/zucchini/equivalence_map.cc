// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/equivalence_map.h"

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
  if (!old_image.IsReference(src) && !new_image.IsReference(dst)) {
    return old_image.GetRawValue(src) == new_image.GetRawValue(dst) ? 1.0
                                                                    : -1.5;
  }

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

EquivalenceCandidate ExtendEquivalenceForward(
    const ImageIndex& old_image,
    const ImageIndex& new_image,
    const EquivalenceCandidate& candidate,
    double min_similarity) {
  EquivalenceCandidate result = candidate;
  offset_t best_offset = candidate.eq.length;
  double current_similarity = candidate.similarity;
  double current_penalty = min_similarity;
  for (offset_t k = best_offset;
       candidate.eq.src_offset + k < old_image.size() &&
       candidate.eq.dst_offset + k < new_image.size();
       ++k) {
    // Mismatch in type, |candidate| cannot be extended further.
    if (old_image.GetType(candidate.eq.src_offset + k) !=
        new_image.GetType(candidate.eq.dst_offset + k))
      break;

    if (!new_image.IsToken(candidate.eq.dst_offset + k)) {
      // Non-tokens are joined with the nearest previous token: skip until we
      // cover the unit, and extend |best_pos| if applicable.
      if (best_offset == k)
        best_offset = k + 1;
      continue;
    }

    DCHECK(old_image.GetType(candidate.eq.src_offset + k) ==
           new_image.GetType(candidate.eq.dst_offset + k));  // Sanity check.
    double similarity =
        GetTokenSimilarity(old_image, new_image, candidate.eq.src_offset + k,
                           candidate.eq.dst_offset + k);
    current_similarity += similarity;
    current_penalty = std::max(0.0, current_penalty) - similarity;

    if (current_similarity < 0.0 || current_penalty >= min_similarity)
      break;
    if (current_similarity >= result.similarity) {
      result.similarity = current_similarity;
      best_offset = k + 1;
    }
  }

  result.eq.length = best_offset;
  return result;
}

EquivalenceCandidate ExtendEquivalenceBackward(
    const ImageIndex& old_image,
    const ImageIndex& new_image,
    const EquivalenceCandidate& candidate,
    double min_similarity) {
  EquivalenceCandidate result = candidate;
  offset_t best_offset = 0;
  double current_similarity = candidate.similarity;
  double current_penalty = 0.0;
  for (offset_t k = 1;
       k <= candidate.eq.dst_offset && k <= candidate.eq.src_offset; ++k) {
    // Mismatch in type, |candidate| cannot be extended further.
    if (old_image.GetType(candidate.eq.src_offset - k) !=
        new_image.GetType(candidate.eq.dst_offset - k))
      break;

    // Non-tokens are joined with the nearest previous token: skip until we
    // reach the next token.
    if (!new_image.IsToken(candidate.eq.dst_offset - k))
      continue;

    DCHECK(old_image.GetType(candidate.eq.src_offset - k) ==
           new_image.GetType(candidate.eq.dst_offset - k));  // Sanity check.
    double similarity =
        GetTokenSimilarity(old_image, new_image, candidate.eq.src_offset - k,
                           candidate.eq.dst_offset - k);
    current_similarity += similarity;
    current_penalty = std::max(0.0, current_penalty) - similarity;

    if (current_similarity < 0.0 || current_penalty >= min_similarity)
      break;
    if (current_similarity >= result.similarity) {
      result.similarity = current_similarity;
      best_offset = k;
    }
  }

  result.eq.dst_offset -= best_offset;
  result.eq.src_offset -= best_offset;
  result.eq.length += best_offset;
  return result;
}

EquivalenceCandidate VisitEquivalenceSeed(const ImageIndex& old_image,
                                          const ImageIndex& new_image,
                                          offset_t src,
                                          offset_t dst,
                                          double min_similarity) {
  EquivalenceCandidate candidate{{src, dst, 0}, 0.0};  // Empty.
  candidate =
      ExtendEquivalenceForward(old_image, new_image, candidate, min_similarity);
  if (candidate.similarity < min_similarity)
    return candidate;  // Not worth exploring any more.
  candidate = ExtendEquivalenceBackward(old_image, new_image, candidate,
                                        min_similarity);
  return candidate;
}

EquivalenceMap::EquivalenceMap() = default;
EquivalenceMap::~EquivalenceMap() = default;

void EquivalenceMap::Build(const std::vector<offset_t>& old_sa,
                           const ImageIndex& old_image,
                           const ImageIndex& new_image,
                           double min_similarity) {
  DCHECK_EQ(old_sa.size(), old_image.size());

  FindCandidates(old_sa, old_image, new_image, min_similarity);

  // Sort by destination
  std::sort(candidates_.begin(), candidates_.end(),
            [](const EquivalenceCandidate& a, const EquivalenceCandidate& b) {
              return a.eq.dst_offset < b.eq.dst_offset;
            });

  Prune(old_image, new_image, min_similarity);

  offset_t coverage = 0;
  offset_t current_offset = 0;
  for (auto candidate : candidates_) {
    DCHECK_GE(candidate.eq.dst_offset, current_offset);
    coverage += candidate.eq.length;
    current_offset = candidate.eq.dst_offset + candidate.eq.length;
  }
  LOG(INFO) << "Equivalence Count: " << size();
  LOG(INFO) << "Coverage / Extra / Total: " << coverage << " / "
            << new_image.size() - coverage << " / " << new_image.size();
}

void EquivalenceMap::FindCandidates(const std::vector<offset_t>& old_sa,
                                    const ImageIndex& old_image,
                                    const ImageIndex& new_image,
                                    double min_similarity) {
  // We are building a suffix array of |old_view| to find seeds of Equivalences.
  candidates_.clear();

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
    double best_similarity = min_similarity;
    EquivalenceCandidate best_candidate = {{0, 0, 0}, 0.0};
    for (auto it = match; it != old_sa.end(); ++it) {
      EquivalenceCandidate candidate =
          VisitEquivalenceSeed(old_image, new_image, static_cast<offset_t>(*it),
                               dst_offset, min_similarity);
      if (candidate.similarity > best_similarity) {
        best_candidate = candidate;
        best_similarity = candidate.similarity;
        next_dst_offset = candidate.eq.dst_offset + candidate.eq.length;
      } else {
        break;
      }
    }
    for (auto it = match; it != old_sa.begin(); --it) {
      EquivalenceCandidate candidate = VisitEquivalenceSeed(
          old_image, new_image, static_cast<offset_t>(it[-1]), dst_offset,
          min_similarity);
      if (candidate.similarity > best_similarity) {
        best_candidate = candidate;
        best_similarity = candidate.similarity;
        next_dst_offset = candidate.eq.dst_offset + candidate.eq.length;
      } else {
        break;
      }
    }
    if (best_candidate.similarity >= min_similarity) {
      candidates_.push_back(best_candidate);
    }

    dst_offset = next_dst_offset;
  }
}

void EquivalenceMap::Prune(const ImageIndex& old_image,
                           const ImageIndex& new_image,
                           double min_similarity) {
  for (auto current = candidates_.begin(); current != candidates_.end();
       ++current) {
    if (current->similarity < min_similarity)
      continue;  // This candidate will be discarded anyways.

    // Look ahead to resolve overlaps, until a better candidate is found.
    for (auto next = current + 1; next != candidates_.end(); ++next) {
      DCHECK_GE(next->eq.dst_offset, current->eq.dst_offset);
      if (next->eq.dst_offset >= current->eq.dst_offset + current->eq.length)
        break;  // No more overlap.

      offset_t overlap =
          current->eq.dst_offset + current->eq.length - next->eq.dst_offset;

      // |next| is better, so |current| shrinks.
      if (current->similarity < next->similarity) {
        current->eq.length -= overlap;
        current->similarity = GetEquivalenceSimilarity(
            old_image, new_image,
            Equivalence{current->eq.src_offset, current->eq.dst_offset,
                        current->eq.length});
        break;
      }
    }

    // Shrinks all overlapping candidates following and worse than |current|.
    for (auto next = current + 1; next != candidates_.end(); ++next) {
      if (next->eq.dst_offset >= current->eq.dst_offset + current->eq.length)
        break;  // No more overlap.

      offset_t overlap =
          current->eq.dst_offset + current->eq.length - next->eq.dst_offset;
      next->eq.length =
          next->eq.length > overlap ? next->eq.length - overlap : 0;
      next->eq.src_offset += overlap;
      next->eq.dst_offset += overlap;
      next->similarity = GetEquivalenceSimilarity(
          old_image, new_image,
          {next->eq.src_offset, next->eq.dst_offset, next->eq.length});
      DCHECK_EQ(next->eq.dst_offset,
                current->eq.dst_offset + current->eq.length);
    }
  }

  // Discard all candidates with similarity smaller than |min_similarity|.
  candidates_.erase(
      std::remove_if(candidates_.begin(), candidates_.end(),
                     [min_similarity](const EquivalenceCandidate& candidate) {
                       return candidate.similarity < min_similarity;
                     }),
      candidates_.end());
}

}  // namespace zucchini
