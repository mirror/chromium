// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_PATCH_READER_H_
#define CHROME_INSTALLER_ZUCCHINI_PATCH_READER_H_

#include <cstddef>
#include <cstdint>
#include <vector>

#include "base/optional.h"
#include "chrome/installer/zucchini/buffer_source.h"
#include "chrome/installer/zucchini/buffer_view.h"
#include "chrome/installer/zucchini/image_utils.h"
#include "chrome/installer/zucchini/patch_utils.h"

namespace zucchini {

namespace patch {

// Parses |source| to extract an ElementMatch. If parsing is successfull, writes
// the result into |element_match| and returns true. Otherwise returns false.
bool ParseElementMatch(BufferSource* source, ElementMatch* element_match);

// Parses |source| to extract a BufferSource. If parsing is successfull, writes
// the result into |buffer|. Otherwise returns false.
bool ParseBuffer(BufferSource* source, BufferSource* buffer);

// Parses a VarUInt from |source|. If parsing is successfull, writes the result
// into |value| and returns true. Otherwise returns false.
template <class T>
bool ParseVarUInt(BufferSource* source, T* value) {
  auto it = DecodeVarUInt(source->begin(), source->end(), value);
  if (!it.has_value())
    return false;
  *source = BufferSource::FromRange(it.value(), source->end());
  return true;
}

// Parses a VarInt from |source|. If parsing is successfull, writes the result
// into |value| and returns true. Otherwise returns false.
template <class T>
bool ParseVarInt(BufferSource* source, T* value) {
  auto it = DecodeVarInt(source->begin(), source->end(), value);
  if (!it.has_value())
    return false;
  *source = BufferSource::FromRange(it.value(), source->end());
  return true;
}

}  // namespace patch

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

  // If data read from |source| is well-formed, initialize internal state to
  // read from it, and returns true. Otherwise returns false.
  bool Initialize(BufferSource* source);

  // Returns the next equivalence available, or nullopt if data is
  // exhausted or ill-formed.
  base::Optional<Equivalence> GetNext();

  // Returns true if data is exhausted.
  bool Done() const {
    return src_skip_.empty() && dst_skip_.empty() && copy_count_.empty();
  }

  // Accessors for unittest.
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
 public:
  ExtraDataSource();
  ExtraDataSource(const ExtraDataSource&);
  ~ExtraDataSource();

  // If data read from |source| is well-formed, initialize internal state to
  // read from it, and returns true. Otherwise returns false.
  bool Initialize(BufferSource* source);

  // Returns the next |size| bytes of data, or nullopt if data is
  // exhausted or ill-formed.
  base::Optional<ConstBufferView> GetNext(offset_t size);

  // Accessors for unittest.
  BufferSource extra_data() const { return extra_data_; }

  // Returns true if data is exhausted.
  bool Done() const { return extra_data_.empty(); }

 private:
  BufferSource extra_data_;
};

// Source for raw delta.
class RawDeltaSource {
 public:
  RawDeltaSource();
  RawDeltaSource(const RawDeltaSource&);
  ~RawDeltaSource();

  // If data read from |source| is well-formed, initialize internal state to
  // read from it, and returns true. Otherwise returns false.
  bool Initialize(BufferSource* source);

  // Returns the next raw delta unit available, or nullopt if data is
  // exhausted or ill-formed.
  base::Optional<RawDeltaUnit> GetNext();

  // Returns true if data is exhausted.
  bool Done() const {
    return raw_delta_skip_.empty() && raw_delta_diff_.empty();
  }

  // Accessors for unittest.
  BufferSource raw_delta_skip() const { return raw_delta_skip_; }
  BufferSource raw_delta_diff() const { return raw_delta_diff_; }

 private:
  BufferSource raw_delta_skip_;
  BufferSource raw_delta_diff_;
};

// Source for reference delta.
class ReferenceDeltaSource {
 public:
  ReferenceDeltaSource();
  ReferenceDeltaSource(const ReferenceDeltaSource&);
  ~ReferenceDeltaSource();

  // If data read from |source| is well-formed, initialize internal state to
  // read from it, and returns true. Otherwise returns false.
  bool Initialize(BufferSource* source);

  // Returns the next reference delta available, or nullopt if data is
  // exhausted or ill-formed.
  base::Optional<int32_t> GetNext();

  // Returns true if data is exhausted.
  bool Done() const { return reference_delta_.empty(); }

  // Accessors for unittest.
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

  // If data read from |source| is well-formed, initialize internal state to
  // read from it, and returns true. Otherwise returns false.
  bool Initialize(BufferSource* source);

  // Returns the next target available, or nullopt if data is
  // exhausted or ill-formed.
  base::Optional<offset_t> GetNext();

  // Returns true if data is exhausted.
  bool Done() const { return extra_targets_.empty(); }

  // Accessors for unittest.
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
class PatchElementReader {
 public:
  PatchElementReader();
  PatchElementReader(const PatchElementReader&);
  ~PatchElementReader();

  // If data read from |source| is well-formed, initialize internal state to
  // read from it, and returns true. Otherwise returns false.
  bool Initialize(BufferSource* source);

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
    DCHECK_LT(tag.value(), extra_targets_.size());
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
class EnsemblePatchReader {
 public:
  // If data read from |buffer| is well-formed, initializes and returns
  // an insteance of EnsemblePatchReader. Otherwise returns base::nullopt.
  static base::Optional<EnsemblePatchReader> Create(ConstBufferView buffer);

  EnsemblePatchReader();
  EnsemblePatchReader(const EnsemblePatchReader&);
  ~EnsemblePatchReader();

  // If data read from |source| is well-formed, initialize internal state to
  // read from it, and returns true. Otherwise returns false.
  bool Initialize(BufferSource* source);

  const PatchHeader& header() const { return header_; }
  PatchType patch_type() const { return patch_type_; }

  const std::vector<PatchElementReader>& elements() const { return elements_; }

 private:
  PatchHeader header_;
  PatchType patch_type_;
  std::vector<PatchElementReader> elements_;
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_PATCH_READER_H_
