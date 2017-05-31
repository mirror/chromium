// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "zucchini/equivalence_map.h"

#include <cstddef>
#include <iostream>

namespace zucchini {

EquivalenceMap::ForwardMapper::ForwardMapper(iterator first, iterator last)
    : first_(first), last_(last) {}

EquivalenceMap::ForwardMapper::~ForwardMapper() = default;

EquivalenceMap::ForwardMapper::ForwardMapper(const ForwardMapper&) = default;

EquivalenceMap::ForwardMapper::iterator EquivalenceMap::ForwardMapper::
operator()(offset_t src) {
  while (first_ != last_ && first_->src + first_->length <= src)
    ++first_;

  // At this point, (first_->src + first_->length) > |src|.
  if ((first_ != last_ && first_->src <= src)) {
    return first_;
  }
  return last_;
}

EquivalenceMap::BackwardMapper::BackwardMapper(iterator first, iterator last)
    : first_(first), last_(last) {}

EquivalenceMap::BackwardMapper::~BackwardMapper() = default;

EquivalenceMap::BackwardMapper::BackwardMapper(const BackwardMapper&) = default;

EquivalenceMap::BackwardMapper::iterator EquivalenceMap::BackwardMapper::
operator()(offset_t dst) {
  while (first_ != last_ && dst >= first_->dst + first_->length)
    ++first_;

  // At this point, (first_->dst + first_->length) > |dst|.
  if (first_ != last_ && dst >= first_->dst) {
    return first_;
  }
  return last_;
}

EquivalenceMap::EquivalenceMap() = default;
EquivalenceMap::~EquivalenceMap() = default;

void EquivalenceMap::Build(const EncodedView& old_view,
                           const SuffixArray<const EncodedView&>& old_sa,
                           const EncodedView& new_view,
                           int minimum_length) {
  using std::begin;
  using std::end;
  using size_type = offset_t;
  equivs_.clear();

  // We are building a suffix array of |old_view| to find seeds of Equivalences.
  EncodedView::const_iterator old_it = begin(old_view);
  EncodedView::const_iterator new_it = begin(new_view);
  size_type old_size = ranges::size(old_view);
  size_type new_size = ranges::size(new_view);

  // This is a heuristic to find 'good' Equivalences.
  // Equivalences are found in ascending order of |new_view|.
  size_type dst = 0;
  std::vector<int> previous_scores;
  Equivalence pe = kNullEquivalence;
  while (dst < new_size) {
    if (!new_view.IsToken(dst)) {
      ++dst;
      continue;
    }
    // |current_match| is a seed of Equivalence that will be extended in
    // both direction and might be selected if long enough.
    auto current_match = old_sa.Search(new_it + dst, end(new_view));
    size_type src = size_type(current_match.first);
    size_type length = size_type(current_match.second);
    if (length < kMinMatchLength) {
      ++dst;
      continue;
    }
    auto previous_equiv = equivs_.rbegin();
    // If the most recent equivalence subsumes |current_match| then skip.
    if (previous_equiv != equivs_.rend() &&
        dst < previous_equiv->dst + previous_equiv->length &&
        dst - src == previous_equiv->dst - previous_equiv->src) {
      dst = previous_equiv->dst + previous_equiv->length;
      continue;
    }

    int next_dst = dst + length;
    int current_dst = dst;

    // We extend the |current_match| by scanning backward and finding the best
    // position to stop. By best, we mean to extend the most we can, while
    // keeping accumulated distance low.
    int score = length - kBaseEquivalenceCost;
    int best_score = score;
    int overlap_score = 0;
    size_type best_pos = 0;
    int penalty = 0;
    for (size_type k = 1; k <= dst && k <= src; ++k) {
      if (!new_view.IsToken(dst - k)) {
        // Non-tokens are joined with the nearest previous token: skip until we
        // reach the boss.
        continue;
      }

      if (previous_equiv != equivs_.rend() &&
          dst - k < previous_equiv->dst + previous_equiv->length) {
        // The current equivalence overlaps with a previous equivalence. We are
        // using |previous_scores| to track the best position to terminate this
        // previous equivalence so it does not overlap with the current one.
        previous_scores.resize(previous_equiv->length + 1);
        if (pe.dst != previous_equiv->dst || pe.src != previous_equiv->src) {
          pe = *previous_equiv;
          int pscore = -kBaseEquivalenceCost;
          int best_pscore = pscore;
          for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(pe.length);
               ++i) {
            if (new_view.IsToken(pe.dst + i)) {
              pscore +=
                  1 - new_view.Distance(old_it[pe.src + i], new_it[pe.dst + i]);
              best_pscore = std::max(best_pscore, pscore);
            }
            previous_scores[i + 1] =
                best_pscore >= minimum_length ? best_pscore : 0;
          }
        } else {
          pe.length = previous_equiv->length;
        }

        overlap_score = previous_scores.back();
        ++previous_equiv;
      }

      int dist = new_view.Distance(new_it[dst - k], old_it[src - k]);
      if (dist != kMismatchFatal) {
        score += 1 - dist;
        penalty = std::max(0, penalty - 1) + dist;
      } else {
        break;
      }
      if (penalty >= kBaseEquivalenceCost) {
        break;
      }

      if (pe.dst != kNullOffset) {
        std::ptrdiff_t i = dst - k - pe.dst;
        if (i >= 0 && i < std::ptrdiff_t(previous_scores.size())) {
          score -= (overlap_score - previous_scores[i]);
          overlap_score = previous_scores[i];
        }
      }
      if (score >= best_score) {
        best_score = score;
        best_pos = k;
      }
    }
    dst -= best_pos;
    src -= best_pos;
    length += best_pos;
    score = best_score;

    // We extend the |current_match| by scanning forward and finding the best
    // position to stop. By best, we mean to extend the most we can, while
    // keeping accumulated distance low.
    best_pos = length;
    penalty = 0;
    for (size_type k = length; dst + k < new_size && src + k < old_size; ++k) {
      if (!new_view.IsToken(dst + k)) {
        // Non-tokens are joined with the nearest previous token: skip until we
        // cover the unit, and extend |best_pos| if applicable.
        if (best_pos == k) {
          best_pos = k + 1;
        }
        continue;
      }

      int dist = new_view.Distance(new_it[dst + k], old_it[src + k]);
      if (dist != kMismatchFatal) {
        score += 1 - dist;
        penalty = std::max(0, penalty - 1) + dist;
      } else {
        break;
      }
      if (penalty >= kBaseEquivalenceCost) {
        break;
      }
      if (score >= best_score) {
        best_score = score;
        best_pos = k + 1;
      }
    }
    length = best_pos;

    // If the Equivalence is deemed long enough, we add it to the container and
    // keep looking past its end.
    if (best_score >= minimum_length) {
      previous_equiv = equivs_.rbegin();
      while (previous_equiv != equivs_.rend() && dst < previous_equiv->dst) {
        equivs_.pop_back();
        previous_equiv = equivs_.rbegin();
        pe = kNullEquivalence;
      }
      if (previous_equiv != equivs_.rend() &&
          dst < previous_equiv->dst + previous_equiv->length) {
        previous_equiv->length = dst - previous_equiv->dst;

        int pscore = -kBaseEquivalenceCost;
        int best_pscore = pscore;
        size_type best_ppos = 0;
        for (size_type i = 0; i < previous_equiv->length; ++i) {
          if (!new_view.IsToken(previous_equiv->dst + i)) {
            if (best_ppos == i)
              best_ppos = i + 1;
            continue;
          }
          pscore += 1 - new_view.Distance(old_it[previous_equiv->src + i],
                                          new_it[previous_equiv->dst + i]);
          if (pscore >= best_pscore) {
            best_pscore = pscore;
            best_ppos = i + 1;
          }
        }
        if (best_pscore >= minimum_length) {
          previous_equiv->length = best_ppos;
        } else {
          equivs_.pop_back();
          pe = kNullEquivalence;
        }
      }

      equivs_.push_back({src, dst, length});
      dst = next_dst;
    } else {
      dst = current_dst + 1;
    }
  }

  size_type coverage = 0;
  for (auto e : equivs_) {
    coverage += e.length;
  }
  std::cout << "Equivalence count: " << size() << std::endl;
  std::cout << " Coverage: " << coverage << std::endl;
  std::cout << " Extra: " << new_size - coverage << std::endl;
  SortBySource();
}

}  // namespace zucchini
