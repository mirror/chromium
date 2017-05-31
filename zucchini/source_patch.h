// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_SOURCE_PATCH_H_
#define ZUCCHINI_SOURCE_PATCH_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "zucchini/ensemble.h"
#include "zucchini/equivalence_map.h"
#include "zucchini/image_utils.h"
#include "zucchini/patch.h"
#include "zucchini/region.h"
#include "zucchini/stream.h"

namespace zucchini {

// Utility to process a Zucchini patch, and outer class for generator classes
// that manage stream access. SourcePatch is also used to iterate over elements
// in "new" image that were matched from the "old" image.
class SourcePatch {
 public:
  using Stream = SourceStreamSet::Stream;
  using iterator = Stream::iterator;

  // The generator inner classes are used for structured traversal of streams.
  // Each generator implements operator(), which reads data to a given pointer
  // or range. These return true on success, and false on failure or on
  // end-of-stream.

  // Generates commands by reading from SourcePatch.
  class CommandGenerator {
   public:
    explicit CommandGenerator(Stream command);
    CommandGenerator(const CommandGenerator&);
    bool operator()(uint32_t* v);

   private:
    Stream command_;
  };

  // Generates equivalences by reading from SourcePatch. We delta encoded
  // streams, so |base_equiv| needs to be provided as starting point.
  class EquivalenceGenerator {
   public:
    EquivalenceGenerator(Stream src_skip,
                         Stream dst_skip,
                         Stream copy_count,
                         offset_t threshold,
                         Equivalence base_equiv);
    EquivalenceGenerator(const EquivalenceGenerator&);
    ~EquivalenceGenerator();
    bool operator()(Equivalence* e);

   private:
    Stream src_skip_;
    Stream dst_skip_;
    Stream copy_count_;
    const offset_t threshold_;
    Equivalence cur_equiv_;
  };

  // Generates reference deltas by reading from SourcePatch.
  class ReferenceDeltaGenerator {
   public:
    explicit ReferenceDeltaGenerator(Stream reference_delta);
    ~ReferenceDeltaGenerator();
    bool operator()(int32_t* r);

   private:
    Stream reference_delta_;

    DISALLOW_COPY_AND_ASSIGN(ReferenceDeltaGenerator);
  };

  // Generates extra data by reading from SourcePatch.
  class ExtraDataGenerator {
   public:
    explicit ExtraDataGenerator(Stream extra_data);
    ~ExtraDataGenerator();
    bool operator()(Region r);

   private:
    Stream extra_data_;

    DISALLOW_COPY_AND_ASSIGN(ExtraDataGenerator);
  };

  // Generates raw deltas by reading from SourcePatch.
  class RawDeltaGenerator {
   public:
    RawDeltaGenerator(Stream delta_skip, Stream delta_diff);
    ~RawDeltaGenerator();
    bool operator()(RawDeltaUnit* d);

   private:
    Stream delta_skip_;
    Stream delta_diff_;
    copy_offset_t prev_copy_offset_;

    DISALLOW_COPY_AND_ASSIGN(RawDeltaGenerator);
  };

  // Generates label values by reading from SourcePatch.
  class LabelGenerator {
   public:
    LabelGenerator(Stream* labels, size_t count);
    ~LabelGenerator();
    bool operator()(offset_t* l);

   private:
    Stream* labels_;
    std::ptrdiff_t remaining_;
    offset_t current_ = 0;

    DISALLOW_COPY_AND_ASSIGN(LabelGenerator);
  };

  SourcePatch(SourceStreamSet* streams, offset_t equivalence_threshold);
  ~SourcePatch();

  // Prints error message to console, and returns false.
  bool OutputErrorAndReturnFalse();

  // Returns true if no error has occurred. Otherwise calls
  // OutputErrorAndReturnFalse().
  bool CheckOK();

  PatchType GetPatchType() const { return patch_type_; }
  size_t GetNumElements() const { return num_elements_; }

  // Loads the next matched element inside |new_image|, using data read from
  // patch. On success, returns true, writes the result to |*match|, and updates
  // states that impact local generators (Local*Gen()) below. Otherwise returns
  // false.
  bool LoadNextElement(Region old_image,
                       Region new_image,
                       EnsembleMatcher::Match* match);

  // Returns various generators, all reading from |streams_|. Except for
  // EquivalenceGenerator, generators are "non-renewable", i.e., a value can
  // only be read once with no means to "rewind". Non-renewable generators are
  // implemented by storing generators as member variables, and are returned as
  // pointers in the accessors below. Also, generators can be global or local:
  // - Global generators are used to patch the entire "new" image.
  // - Local generators focus on patching elements in the "new" image, and are
  //   initialized by LoadNextElement().
  EquivalenceGenerator GlobalEquivalenceGen();
  ExtraDataGenerator* GlobalExtraDataGen() { return &extra_data_gen_; }
  RawDeltaGenerator* GlobalRawDeltaGen() { return &raw_delta_gen_; }
  EquivalenceGenerator LocalEquivalenceGen();
  ReferenceDeltaGenerator* LocalReferenceDeltaGen() {
    return &local_reference_delta_gen_;
  }
  LabelGenerator* LocalLabelGen(uint8_t pool) {
    return local_label_gen_list_[pool].get();
  }

 private:
  // For each element, we'd like to generate local equivalences that cover it.
  // A simple solution is to preprocess all equivalences, store them in a
  // list, and emit slices -- but this requires extra storage when we apply
  // patch, which we want to avoid. This class enables memory-efficient local
  // EquivalenceGenerator generation by:
  // - "Marching" over the 3 streams that encode equivalences in one pass.
  // - For each element, extracting a base equivalence and 3 substreams.
  // - Having the owner use extracted data to instantiate local
  //   EquivalenceGenerators.
  // A challenge is that when we read a VarInt from a stream, we'd advance the
  // stream's current position (i.e., you can't have a cake and eat iterator).
  // To solve this, we store |cur_*_it_| iterators that lag marching streams
  // by one VarInt.
  struct LocalEquivalenceScanner {
    LocalEquivalenceScanner(offset_t equivalence_threshold,
                            Stream src_skip,
                            Stream dst_skip,
                            Stream copy_count);
    LocalEquivalenceScanner(const LocalEquivalenceScanner&);
    ~LocalEquivalenceScanner();

    // Advances |next_equiv_| and |cur_equiv_| by one equivalence. Returns
    // false when end-of-data is reached; when this happens, |cur_equiv_|
    // would contain the final equivalence. Returns true otherwise.
    bool AdvanceEquivalence();

    // Advances to the equivalences covered by a "current element" in the "new"
    // image at non-empty range |[start_offset, end_offset)|. Successive calls
    // to this function must provide increasing and non-overlapping ranges.
    // Returns true on success, and false on error.
    bool Advance(offset_t start_offset, offset_t end_offset);

    // Instantiates an EquivalenceGenerator for current chunk. This call is
    // renewable.
    EquivalenceGenerator LocalGen();

   private:
    const offset_t equivalence_threshold_;

    // Streams that march forward, from which we extract local equivalences.
    Stream marching_src_skip_;
    Stream marching_dst_skip_;
    Stream marching_copy_count_;

    // Iterators that lag |marching_*| read positions by one VarInt.
    Stream::iterator cur_src_skip_it_;
    Stream::iterator cur_dst_skip_it_;
    Stream::iterator cur_copy_count_it_;

    // Substreams to store Advance() results.
    Stream local_src_skip_;
    Stream local_dst_skip_;
    Stream local_copy_count_;

    // Stored equivalences.
    Equivalence cur_equiv_ = {0, 0, 0};
    Equivalence next_equiv_ = {0, 0, 0};
    Equivalence base_equiv_ = {0, 0, 0};

    // For error checking.
    offset_t prev_end_offset_ = 0;
  };

  SourceStreamSet* streams_;
  const offset_t equivalence_threshold_;

  // Stored error message. Empty means no error.
  std::string error_message_;

  // Common streams used by all Labels, to make then non-renewable.
  std::vector<Stream> label_streams_;

  // Non-renewable generators.
  CommandGenerator command_gen_;
  ExtraDataGenerator extra_data_gen_;
  RawDeltaGenerator raw_delta_gen_;
  ReferenceDeltaGenerator local_reference_delta_gen_;
  std::vector<std::unique_ptr<LabelGenerator>> local_label_gen_list_;

  // Global patch properties.
  PatchType patch_type_ = NUM_PATCH_TYPES;
  uint32_t num_elements_ = 0;

  // States for visiting elements and instantiating local generators.
  uint32_t cur_element_idx_ = 0;
  LocalEquivalenceScanner equiv_scanner_;

  DISALLOW_COPY_AND_ASSIGN(SourcePatch);
};

}  // namespace zucchini

#endif  // ZUCCHINI_SOURCE_PATCH_H_
