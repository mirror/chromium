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

bool Parse(ElementMatch* element_match, BufferSource* source);

bool Parse(BufferSource* buffer, BufferSource* source);

template <class T>
bool DecodeVarUInt(T* value, BufferSource* source) {
  auto it = DecodeVarUInt(source->begin(), source->end(), value);
  if (!it.has_value())
    return false;
  *source = BufferSource::FromRange(it.value(), source->end());
  return true;
}

template <class T>
bool DecodeVarInt(T* value, BufferSource* source) {
  auto it = DecodeVarInt(source->begin(), source->end(), value);
  if (!it.has_value())
    return false;
  *source = BufferSource::FromRange(it.value(), source->end());
  return true;
}

// Generates equivalences by reading from a source buffer.
class EquivalenceSource {
 public:
  EquivalenceSource();
  EquivalenceSource(const EquivalenceSource&);
  ~EquivalenceSource();

  bool Parse(BufferSource* source);

  base::Optional<Equivalence> GetNext();
  bool Done() const;

 private:
  BufferSource src_skip_;
  BufferSource dst_skip_;
  BufferSource copy_count_;

  offset_t previous_src_offset_ = 0;
  offset_t previous_dst_offset_ = 0;
};

// Generates extra data by reading from a source buffer.
class ExtraDataSource {
  friend class ElementPatchSource;

 public:
  ExtraDataSource();
  ExtraDataSource(const ExtraDataSource&);
  ~ExtraDataSource();

  base::Optional<BufferView> GetNext(offset_t size);
  bool Done() const;

 private:
  bool Parse(BufferSource* source);

  BufferSource extra_data_;
};

// Generates raw deltas by reading from SourcePatch.
class RawDeltaSource {
  friend class ElementPatchSource;

 public:
  RawDeltaSource();
  RawDeltaSource(const RawDeltaSource&);
  ~RawDeltaSource();

  bool Parse(BufferSource* source);

  base::Optional<RawDeltaUnit> GetNext();
  bool Done() const;

 private:
  BufferSource raw_delta_skip_;
  BufferSource raw_delta_diff_;
};

// Provides streaming access to reference deltas contained in a patch element.
class ReferenceDeltaSource {
  friend class ElementPatchSource;

 public:
  ReferenceDeltaSource();
  ReferenceDeltaSource(const ReferenceDeltaSource&);
  ~ReferenceDeltaSource();

  bool Parse(BufferSource* source);

  base::Optional<int32_t> GetNext();
  bool Done() const;

 private:
  BufferSource reference_delta_;
};

// This class behaves like a cursor, providing read access to extra targets
// found in a patch element. It reads directly from a memory source given upon
// Parse(), which is expected to remains valid for the lifetime of the object
// using it.
class TargetSource {
  friend class ElementPatchSource;

 public:
  TargetSource();
  TargetSource(const TargetSource&);
  ~TargetSource();

  bool Parse(BufferSource* source);

  base::Optional<offset_t> GetNext();
  bool Done() const;

 private:
  BufferSource extra_symbols_;

  offset_t previous_symbol_ = 0;
};

// Utility to read a patch element. A patch element contains all the information
// necessary to patch a single element. This class provide access to the
// multiple streams of data forming the patch element.
class ElementPatchSource {
  friend class EnsemblePatchSource;
  friend class PatchContainerSource;

 public:
  ElementPatchSource();
  ElementPatchSource(const ElementPatchSource&);
  ~ElementPatchSource();

  ElementMatch GetElementMatch() const;
  size_t GetPoolCount() const;

  EquivalenceSource GetEquivalenceSource() const { return equivalences_; }
  ExtraDataSource GetExtraDataSource() const { return extra_data_; }
  RawDeltaSource GetRawDeltaSource() const { return raw_delta_; }
  ReferenceDeltaSource GetReferenceDeltaSource() const {
    return reference_delta_;
  }
  TargetSource GetExtraTargetSource(PoolTag tag) const {
    return extra_targets_.at(tag);
  }

 private:
  bool Parse(BufferSource* source);

  ElementMatch element_match_;
  EquivalenceSource equivalences_;
  ExtraDataSource extra_data_;
  RawDeltaSource raw_delta_;
  ReferenceDeltaSource reference_delta_;
  std::map<PoolTag, TargetSource> extra_targets_;
};

// Utility to read a Zucchini ensemble patch. An ensemble patch is the
// concatenation of a patch header with a vector of patch elements.
class EnsemblePatchSource {
  friend class PatchContainerSource;

 public:
  static base::Optional<EnsemblePatchSource> Create(BufferView buffer);

  EnsemblePatchSource();
  EnsemblePatchSource(const EnsemblePatchSource&);
  ~EnsemblePatchSource();

  const PatchHeader& GetHeader() const { return header_; }

  const std::vector<ElementPatchSource>& GetElements() const {
    return elements_;
  }

 private:
  PatchHeader header_;
  bool Parse(BufferSource* source);

  std::vector<ElementPatchSource> elements_;
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_SOURCE_PATCH_H_
