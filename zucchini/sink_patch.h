// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_SINK_PATCH_H_
#define ZUCCHINI_SINK_PATCH_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "base/macros.h"
#include "zucchini/ensemble.h"
#include "zucchini/equivalence_map.h"
#include "zucchini/image_utils.h"
#include "zucchini/patch.h"
#include "zucchini/region.h"
#include "zucchini/stream.h"

namespace zucchini {

// Utility to write a Zucchini patch, and outer class for receptor classes
// that manage stream access.
class SinkPatch {
 public:
  using Stream = SinkStreamSet::Stream;
  using iterator = Stream::iterator;

  // The receptor inner classes are used for structured writing of streams. Each
  // receptor implements operator(), which writes given data to underlying sink
  // streams.

  // Receives commands.
  class CommandReceptor {
   public:
    explicit CommandReceptor(Stream command);
    void operator()(uint32_t v);

   private:
    Stream command_;

    DISALLOW_COPY_AND_ASSIGN(CommandReceptor);
  };

  // Receives equivalences with callable operator and writes them to SrcPatch.
  class EquivalenceReceptor {
   public:
    EquivalenceReceptor(Stream src_skip,
                        Stream dst_skip,
                        Stream copy_count,
                        offset_t threshold);
    void operator()(const Equivalence& e);

   private:
    Stream src_skip_;
    Stream dst_skip_;
    Stream copy_count_;
    const offset_t threshold_;
    // These keep track of previous end-of-read offsets, for delta encoding.
    offset_t equivalence_src_ = 0;
    offset_t equivalence_dst_ = 0;

    DISALLOW_COPY_AND_ASSIGN(EquivalenceReceptor);
  };

  // Receives extra data with callable operator and writes them to SrcPatch.
  class ExtraDataReceptor {
   public:
    explicit ExtraDataReceptor(Stream extra_data);
    void operator()(Region r);

   private:
    Stream extra_data_;

    DISALLOW_COPY_AND_ASSIGN(ExtraDataReceptor);
  };

  // Receives raw deltas with callable operator and writes them to SrcPatch.
  class RawDeltaReceptor {
   public:
    RawDeltaReceptor(Stream delta_skip, Stream delta_diff);
    void operator()(RawDeltaUnit d);

    copy_offset_t GetBaseCopyOffset() const { return base_copy_offset_; }
    void SetBaseCopyOffset(copy_offset_t v) { base_copy_offset_ = v; }

   private:
    Stream delta_skip_;
    Stream delta_diff_;
    // Copy offsets are computed by accumulating lengths of Equivalences while
    // visiting them in "new" image order. We track this in |base_copy_offset_|,
    // and work with callers to compute current copy offset.
    copy_offset_t base_copy_offset_ = 0;
    // The previous copy offset (shifted), used for delta encoding.
    copy_offset_t prev_copy_offset_ = 0;

    DISALLOW_COPY_AND_ASSIGN(RawDeltaReceptor);
  };

  // Receives reference deltas with callable operator and writes them to
  // SrcPatch.
  class ReferenceDeltaReceptor {
   public:
    explicit ReferenceDeltaReceptor(Stream reference_delta);
    void operator()(int32_t diff);

   private:
    Stream reference_delta_;

    DISALLOW_COPY_AND_ASSIGN(ReferenceDeltaReceptor);
  };

  // Receives label values with callable operator and writes them to SrcPatch.
  class LabelReceptor {
   public:
    LabelReceptor(Stream labels, size_t count);
    void operator()(uint32_t l);
    std::ptrdiff_t GetRemaining() { return remaining_; }

   private:
    Stream labels_;
    std::ptrdiff_t remaining_;
    offset_t current_ = 0;

    DISALLOW_COPY_AND_ASSIGN(LabelReceptor);
  };

  SinkPatch(SinkStreamSet* streams, offset_t equivalence_threshold);
  ~SinkPatch();

  // Write basic patch info, and store info for later use.
  void WritePatchType(PatchType patch_type);
  void WriteNumElements(size_t num_elements);

  // Starts emitting data for a matched element. Emits basic info and
  // initializes local Label receptors.
  bool StartElementWrite(const Ensemble::Element& old_element,
                         const Ensemble::Element& new_element,
                         uint32_t max_num_pool);

  // Ends emitting data for current element, with cleanup. In particular, writes
  // length of 0 to unused Label streams for the executable.
  bool EndElementWrite();

  // Returns various receptor, all writing to |streams_|. These are all non-
  // renewable, i.e., once written, data do not get modified. This is reflected
  // by making receptors member variables, and by returning pointers in the
  // accessors below. This complements pointer usage in SrcPatch, and is also
  // useful for receptors with states, e.g., RawDeltaReceptor for delta
  // encoding.
  EquivalenceReceptor* Equivalences() { return &equivalence_rec_; }
  ExtraDataReceptor* ExtraData() { return &extra_data_rec_; }
  RawDeltaReceptor* RawDelta() { return &raw_delta_rec_; }
  ReferenceDeltaReceptor* ReferenceDelta() { return &reference_delta_rec_; }
  // Caller commits to writing exactly |count| Labels. On first call for a given
  // |pool| after each StartElementWrite() call, this |count| is emitted to the
  // corresponding Label stream.
  LabelReceptor* LocalLabels(uint8_t pool, size_t count);

 private:
  SinkStreamSet* streams_;
  const offset_t equivalence_threshold_;

  PatchType patch_type_ = NUM_PATCH_TYPES;
  size_t num_elements_ = 0;

  CommandReceptor command_rec_;
  EquivalenceReceptor equivalence_rec_;
  ExtraDataReceptor extra_data_rec_;
  RawDeltaReceptor raw_delta_rec_;
  ReferenceDeltaReceptor reference_delta_rec_;
  std::vector<std::unique_ptr<LabelReceptor>> label_rec_list_;

  DISALLOW_COPY_AND_ASSIGN(SinkPatch);
};

}  // namespace zucchini

#endif  // ZUCCHINI_SINK_PATCH_H_
