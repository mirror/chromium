// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_SOURCE_PATCH_H_
#define CHROME_INSTALLER_ZUCCHINI_SOURCE_PATCH_H_

#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

#include "base/optional.h"
#include "chrome/installer/zucchini/buffer_source.h"
#include "chrome/installer/zucchini/buffer_view.h"
#include "chrome/installer/zucchini/image_utils.h"
#include "chrome/installer/zucchini/patch_utils.h"

namespace zucchini {

// Parses |source| to extract ElementMatch, which is written into
// |element_match|, moving forward |source|. Returns true if successfull, or
// false if input is ill- formed.
bool Parse(ElementMatch* element_match, BufferSource* source);

// Parses |source| to extract BufferSource, which is written into |buffer|,
// moving forward |source|. Returns true if successfull, or false if input is
// ill-formed.
bool Parse(BufferSource* buffer, BufferSource* source);

// Parses a VarUInt from |source| and writes result into |value|, moving
// forward |source|. Returns true if successfull, or false if input is ill-
// formed.
template <class T>
bool ParseVarUInt(T* value, BufferSource* source) {
  auto it = DecodeVarUInt(source->begin(), source->end(), value);
  if (!it.has_value())
    return false;
  *source = BufferSource::FromRange(it.value(), source->end());
  return true;
}

// Parses a VarInt from |source| and writes result into |value|, moving
// forward |source|. Returns true if successfull, or false if input is ill-
// formed.
template <class T>
bool ParseVarInt(T* value, BufferSource* source) {
  auto it = DecodeVarInt(source->begin(), source->end(), value);
  if (!it.has_value())
    return false;
  *source = BufferSource::FromRange(it.value(), source->end());
  return true;
}

// Following classes behave like input streams, providing read access with the
// method GetNext() to structured data found in a patch element. It reads
// directly from a memory source given upon Parse(), which is expected to
// remains valid for the lifetime of the object using it. Once all data is
// exhausted, Done() returns true.

// Source for equivalences.
class EquivalenceSource {
 public:
  EquivalenceSource();
  EquivalenceSource(const EquivalenceSource&);
  ~EquivalenceSource();

  // Initialize internal state to read data from |source|. Returns true if
  // successfull, or false if data is ill-formed.
  bool Parse(BufferSource* source);

  // Returns the next equivalence available, or nullopt if data is
  // exhausted or ill-formed.
  base::Optional<Equivalence> GetNext();

  // Returns true if data is exhausted.
  bool Done() const;

  BufferSource src_skip() const { return src_skip_; }
  BufferSource dst_skip() const { return dst_skip_; }
  BufferSource copy_count() const { return copy_count_; }

 private:
  BufferSource src_skip_;
  BufferSource dst_skip_;
  BufferSource copy_count_;

  offset_t previous_src_offset_ = 0;
  offset_t previous_dst_offset_ = 0;
};

// Source for extra data.
class ExtraDataSource {
  friend class ElementPatchSource;

 public:
  ExtraDataSource();
  ExtraDataSource(const ExtraDataSource&);
  ~ExtraDataSource();

  // Initialize internal state to read data from |source|. Returns true if
  // successfull, or false if data is ill-formed.
  bool Parse(BufferSource* source);

  // Returns the next |size| bytes of data, or nullopt if data is
  // exhausted or ill-formed.
  base::Optional<ConstBufferView> GetNext(offset_t size);

  BufferSource extra_data() const { return extra_data_; }

  // Returns true if data is exhausted.
  bool Done() const;

 private:
  BufferSource extra_data_;
};

// Source for raw delta.
class RawDeltaSource {
  friend class ElementPatchSource;

 public:
  RawDeltaSource();
  RawDeltaSource(const RawDeltaSource&);
  ~RawDeltaSource();

  // Initialize internal state to read data from |source|. Returns true if
  // successfull, or false if data is ill-formed.
  bool Parse(BufferSource* source);

  // Returns the next raw delta unit available, or nullopt if data is
  // exhausted or ill-formed.
  base::Optional<RawDeltaUnit> GetNext();

  // Returns true if data is exhausted.
  bool Done() const;

  BufferSource raw_delta_skip() const { return raw_delta_skip_; }
  BufferSource raw_delta_diff() const { return raw_delta_diff_; }

 private:
  BufferSource raw_delta_skip_;
  BufferSource raw_delta_diff_;
};

// Source for reference delta.
class ReferenceDeltaSource {
  friend class ElementPatchSource;

 public:
  ReferenceDeltaSource();
  ReferenceDeltaSource(const ReferenceDeltaSource&);
  ~ReferenceDeltaSource();

  // Initialize internal state to read data from |source|. Returns true if
  // successfull, or false if data is ill-formed.
  bool Parse(BufferSource* source);

  // Returns the next reference delta available, or nullopt if data is
  // exhausted or ill-formed.
  base::Optional<int32_t> GetNext();

  // Returns true if data is exhausted.
  bool Done() const;

  BufferSource reference_delta() const { return reference_delta_; }

 private:
  BufferSource reference_delta_;
};

// Source for additional targets.
class TargetSource {
 public:
  TargetSource();
  TargetSource(const TargetSource&);
  ~TargetSource();

  // Initialize internal state to read data from |source|. Returns true if
  // successfull, or false if data is ill-formed.
  bool Parse(BufferSource* source);

  // Returns the next target available, or nullopt if data is
  // exhausted or ill-formed.
  base::Optional<offset_t> GetNext();

  // Returns true if data is exhausted.
  bool Done() const;

  BufferSource extra_targets() const { return extra_targets_; }

 private:
  BufferSource extra_targets_;

  offset_t previous_target_ = 0;
};

// Following are stateless utility classes providing a structured view
// on data forming a patch.

// Utility to read a patch element. A patch element contains all the information
// necessary to patch a single element. This class provide access to the
// multiple streams of data forming the patch element.
class PatchElementSource {
 public:
  PatchElementSource();
  PatchElementSource(const PatchElementSource&);
  ~PatchElementSource();

  // Initialize internal state to read data from |source|. Returns true if
  // successfull, or false if data is ill-formed.
  bool Parse(BufferSource* source);

  const ElementMatch& element_match() const { return element_match_; }
  size_t pool_count() const { return extra_targets_.size(); }

  const EquivalenceSource& GetEquivalenceSource() const {
    return equivalences_;
  }
  const ExtraDataSource& GetExtraDataSource() const { return extra_data_; }
  const RawDeltaSource& GetRawDeltaSource() const { return raw_delta_; }
  const ReferenceDeltaSource& GetReferenceDeltaSource() const {
    return reference_delta_;
  }
  const TargetSource& GetExtraTargetSource(PoolTag tag) const {
    DCHECK(tag.value() < extra_targets_.size());
    return extra_targets_[tag.value()];
  }

 private:
  ElementMatch element_match_;
  EquivalenceSource equivalences_;
  ExtraDataSource extra_data_;
  RawDeltaSource raw_delta_;
  ReferenceDeltaSource reference_delta_;
  std::vector<TargetSource> extra_targets_;
};

// Utility to read a Zucchini ensemble patch. An ensemble patch is the
// concatenation of a patch header with a vector of patch elements.
class EnsemblePatchSource {
 public:
  static base::Optional<EnsemblePatchSource> Create(ConstBufferView buffer);

  EnsemblePatchSource();
  EnsemblePatchSource(const EnsemblePatchSource&);
  ~EnsemblePatchSource();

  // Initialize internal state to read data from |source|. Returns true if
  // successfull, or false if data is ill-formed.
  bool Parse(BufferSource* source);

  const PatchHeader& header() const { return header_; }

  const std::vector<PatchElementSource>& elements() const { return elements_; }

 private:
  PatchHeader header_;
  std::vector<PatchElementSource> elements_;
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_SOURCE_PATCH_H_
