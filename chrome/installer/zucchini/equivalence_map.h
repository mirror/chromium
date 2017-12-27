// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_EQUIVALENCE_MAP_H_
#define CHROME_INSTALLER_ZUCCHINI_EQUIVALENCE_MAP_H_

#include <stddef.h>

#include <limits>
#include <vector>

#include "chrome/installer/zucchini/image_index.h"
#include "chrome/installer/zucchini/image_utils.h"
#include "chrome/installer/zucchini/targets_affinity.h"

namespace zucchini {

class EncodedView;
class ImageIndex;
class EquivalenceSource;

// Returns a similarity score between content in |old_image_index| and
// |new_image_index| at offsets |src| and |dst|, respectively.
// |targets_affinities| describes affinities for each target pool and is used to
// evaluate similarity between references, hence it's size must be equal to the
// number of pools in both |old_image_index| and |new_image_index|. Both |src|
// and |dst| must refer to tokens in |old_image_index| and |new_image_index|.
double GetTokenSimilarity(
    const ImageIndex& old_image_index,
    const ImageIndex& new_image_index,
    const std::vector<TargetsAffinity>& targets_affinities,
    offset_t src,
    offset_t dst);

// Returns a similarity score between content in |old_image_index| and
// |new_image_index| at regions described by |equivalence|, using
// |targets_affinities| to evaluate similarity between references.
double GetEquivalenceSimilarity(
    const ImageIndex& old_image_index,
    const ImageIndex& new_image_index,
    const std::vector<TargetsAffinity>& targets_affinities,
    const Equivalence& equivalence);

// Extends |equivalence| forward and returns the result. This is related to
// VisitEquivalenceSeed().
EquivalenceCandidate ExtendEquivalenceForward(
    const ImageIndex& old_image_index,
    const ImageIndex& new_image_index,
    const std::vector<TargetsAffinity>& targets_affinities,
    const EquivalenceCandidate& equivalence,
    double min_similarity);

// Extends |equivalence| backward and returns the result. This is related to
// VisitEquivalenceSeed().
EquivalenceCandidate ExtendEquivalenceBackward(
    const ImageIndex& old_image_index,
    const ImageIndex& new_image_index,
    const std::vector<TargetsAffinity>& targets_affinities,
    const EquivalenceCandidate& equivalence,
    double min_similarity);

// Creates an equivalence, starting with |src| and |dst| as offset hint, and
// extends it both forward and backward, trying to maximise similarity between
// |old_image_index| and |new_image_index|, and returns the result.
// |targets_affinities| is used to evaluate similarity between references.
// |min_similarity| describes the minimum acceptable similarity score and is
// used as threshold to discard bad equivalences.
EquivalenceCandidate VisitEquivalenceSeed(
    const ImageIndex& old_image_index,
    const ImageIndex& new_image_index,
    const std::vector<TargetsAffinity>& targets_affinities,
    offset_t src,
    offset_t dst,
    double min_similarity);

// Container of equivalences between |old_image| and |new_image|, sorted by
// |Equivalence::dst_offset|, only used during patch generation.
class SimilarityMap {
 public:
  using const_iterator = std::vector<EquivalenceCandidate>::const_iterator;

  SimilarityMap();
  // Initializes the object with |equivalences|.
  explicit SimilarityMap(std::vector<EquivalenceCandidate>&& candidates);
  SimilarityMap(SimilarityMap&&);
  SimilarityMap(const SimilarityMap&) = delete;
  ~SimilarityMap();

  // Finds relevant equivalences between |old_view| and |new_view|, using
  // suffix array |old_sa| computed from |old_view| and using
  // |targets_affinities| to evaluate similarity between references. This
  // function is not symmetric. Equivalences might overlap in |old_view|, but
  // not in |new_view|. It tries to maximize accumulated similarity within each
  // equivalence, while maximizing |new_view| coverage. The minimum similarity
  // of an equivalence is given by |min_similarity|.
  void Build(const std::vector<offset_t>& old_sa,
             const EncodedView& old_view,
             const EncodedView& new_view,
             const std::vector<TargetsAffinity>& targets_affinities,
             double min_similarity);

  size_t size() const { return candidates_.size(); }
  const_iterator begin() const { return candidates_.begin(); }
  const_iterator end() const { return candidates_.end(); }

 private:
  // Discovers equivalence candidates between |old_view| and |new_view| and
  // stores them in the object. Note that resulting candidates are not sorted
  // and might be overlapping in new image.
  void CreateCandidates(const std::vector<offset_t>& old_sa,
                        const EncodedView& old_view,
                        const EncodedView& new_view,
                        const std::vector<TargetsAffinity>& targets_affinities,
                        double min_similarity);
  // Sorts candidates by their offset in new image.
  void SortByDestination();
  // Visits |candidates_| (sorted by |dst_offset|) and remove all destination
  // overlaps. Candidates with low similarity scores are more likely to be
  // shrunken. Unfit candidates may be removed.
  void Prune(const EncodedView& old_view,
             const EncodedView& new_view,
             const std::vector<TargetsAffinity>& targets_affinities,
             double min_similarity);

  std::vector<EquivalenceCandidate> candidates_;
};

class EquivalenceMap {
 public:
  using const_iterator = std::vector<Equivalence>::const_iterator;

  explicit EquivalenceMap(EquivalenceSource&& equivalences);
  explicit EquivalenceMap(const SimilarityMap&);
  ~EquivalenceMap();

  size_t size() const { return equivalences_.size(); }
  const_iterator begin() const { return equivalences_.begin(); }
  const_iterator end() const { return equivalences_.end(); }

  void ProjectOffsets(std::vector<offset_t>* offsets) const {
    auto current_equivalence = begin();
    for (auto& src : *offsets) {
      while (current_equivalence != end() &&
             current_equivalence->src_end() <= src)
        ++current_equivalence;

      if (current_equivalence != end() &&
          current_equivalence->src_offset <= src) {
        src = src - current_equivalence->src_offset +
              current_equivalence->dst_offset;
      } else {
        src = offset_t(-1);
      }
    }
    offsets->erase(std::remove(offsets->begin(), offsets->end(), -1),
                   offsets->end());
  }

  offset_t ProjectOffset(offset_t offset) const {
    auto pos = std::upper_bound(
        equivalences_.begin(), equivalences_.end(), offset,
        [](offset_t a, const Equivalence& b) { return a < b.src_offset; });
    if (pos != equivalences_.begin())
      --pos;
    return offset - pos->src_offset + pos->dst_offset;
  }

 private:
  // Sorts candidates by their offset in new image.
  void SortBySource();
  void Prune();

  std::vector<Equivalence> equivalences_;
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_EQUIVALENCE_MAP_H_
