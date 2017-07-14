// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_PATCH_SINK_H_
#define CHROME_INSTALLER_ZUCCHINI_PATCH_SINK_H_

#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

#include "chrome/installer/zucchini/buffer_sink.h"
#include "chrome/installer/zucchini/buffer_view.h"
#include "chrome/installer/zucchini/crc32.h"
#include "chrome/installer/zucchini/image_utils.h"
#include "chrome/installer/zucchini/patch_utils.h"

namespace zucchini {

// If sufficient space is available, serializes |element_match| into |sink| and
// returns true. Otherwise returns false.
bool SerializeInto(ElementMatch element_match, BufferSink* sink);

// Returns the size in bytes required to serialized |element_match|.
size_t SerializedSize(ElementMatch element_match);

// If sufficient space is available, serializes |buffer| into |sink| and returns
// true. Otherwise returns false.
bool SerializeInto(const std::vector<uint8_t>& buffer, BufferSink* sink);

// Returns the size in bytes required to serialized |buffer|.
size_t SerializedSize(const std::vector<uint8_t>& buffer);

// Following classes behave like output streams, providing write capability of
// structured data with the method PutNext(). It writes into internal buffer
// that can then be serialized with the method SerializeInto(buffer) to a buffer
// large enough to hold SerializedSize() bytes.

// Sink for equivalences.
class EquivalenceSink {
 public:
  EquivalenceSink();
  explicit EquivalenceSink(const std::vector<uint8_t>& src_skip,
                           const std::vector<uint8_t>& dst_skip,
                           const std::vector<uint8_t>& copy_count);

  EquivalenceSink(EquivalenceSink&&);
  ~EquivalenceSink();

  EquivalenceSink& operator=(EquivalenceSink&&) = default;

  // Writes the next |equivalence| into the object.
  void PutNext(const Equivalence& equivalence);

  // Returns the serialized size in bytes of the data this object is holding.
  size_t SerializedSize() const;

  // If sufficient space is available, serializes data into |sink|, which is at
  // least SerializedSize() bytes and returns true. Otherwise returns false.
  bool SerializeInto(BufferSink* buffer) const;

 private:
  std::vector<uint8_t> src_skip_;
  std::vector<uint8_t> dst_skip_;
  std::vector<uint8_t> copy_count_;

  offset_t src_offset_ = 0;
  offset_t dst_offset_ = 0;
};

// Sink for extra data.
class ExtraDataSink {
 public:
  ExtraDataSink();
  explicit ExtraDataSink(const std::vector<uint8_t>& extra_data);
  ExtraDataSink(ExtraDataSink&&);
  ~ExtraDataSink();

  ExtraDataSink& operator=(ExtraDataSink&&) = default;

  // Writes bytes from |region| into the object.
  void PutNext(ConstBufferView region);

  // Returns the serialized size in bytes of the data this object is holding.
  size_t SerializedSize() const;

  // If sufficient space is available, serializes data into |sink|, which is at
  // least SerializedSize() bytes and returns true. Otherwise returns false.
  bool SerializeInto(BufferSink* sink) const;

 private:
  std::vector<uint8_t> extra_data_;
};

// Sink for raw delta.
class RawDeltaSink {
 public:
  RawDeltaSink();
  explicit RawDeltaSink(const std::vector<uint8_t>& raw_delta_skip,
                        const std::vector<uint8_t>& raw_delta_diff);
  RawDeltaSink(RawDeltaSink&&);
  ~RawDeltaSink();

  RawDeltaSink& operator=(RawDeltaSink&&) = default;

  // Writes the next |delta| into the object.
  void PutNext(RawDeltaUnit delta);

  // Returns the serialized size in bytes of the data this object is holding.
  size_t SerializedSize() const;

  // If sufficient space is available, serializes data into |sink|, which is at
  // least SerializedSize() bytes and returns true. Otherwise returns false.
  bool SerializeInto(BufferSink* sink) const;

 private:
  std::vector<uint8_t> raw_delta_skip_;
  std::vector<uint8_t> raw_delta_diff_;
};

// Sink for reference delta.
class ReferenceDeltaSink {
 public:
  ReferenceDeltaSink();
  explicit ReferenceDeltaSink(const std::vector<uint8_t>& reference_delta);
  ReferenceDeltaSink(ReferenceDeltaSink&&);
  ~ReferenceDeltaSink();

  ReferenceDeltaSink& operator=(ReferenceDeltaSink&&) = default;

  // Writes the next |diff| into the object.
  void PutNext(int32_t diff);

  // Returns the serialized size in bytes of the data this object is holding.
  size_t SerializedSize() const;

  // If sufficient space is available, serializes data into |sink|, which is at
  // least SerializedSize() bytes and returns true. Otherwise returns false.
  bool SerializeInto(BufferSink* sink) const;

 private:
  std::vector<uint8_t> reference_delta_;
};

// Sink for additional targets.
class TargetSink {
 public:
  TargetSink();
  explicit TargetSink(const std::vector<uint8_t>& extra_targets);
  TargetSink(TargetSink&&);
  ~TargetSink();

  TargetSink& operator=(TargetSink&&) = default;

  // Writes the next |target| into the object.
  void PutNext(uint32_t target);

  // Returns the serialized size in bytes of the data this object is holding.
  size_t SerializedSize() const;

  // If sufficient space is available, serializes data into |sink|, which is at
  // least SerializedSize() bytes and returns true. Otherwise returns false.
  bool SerializeInto(BufferSink* sink) const;

 private:
  std::vector<uint8_t> extra_targets_;

  offset_t previous_target_ = 0;
};

// Following are utility classes to serialize structured data forming
// a patch.

// Utility to write a patch element. A patch element contains all the
// information necessary to patch a single element. This class provides an
// interface to individually set different building blocks of data in the patch
// element.
class PatchElementSink {
 public:
  PatchElementSink();
  explicit PatchElementSink(ElementMatch element_match);
  PatchElementSink(PatchElementSink&&);
  ~PatchElementSink();

  PatchElementSink& operator=(PatchElementSink&&) = default;

  // Following methods set individual blocks for this element. Previous
  // corresponding block is replaced.

  void SetEquivalenceSink(EquivalenceSink&& equivalences) {
    equivalences_ = std::move(equivalences);
  }
  void SetExtraDataSink(ExtraDataSink&& extra_data) {
    extra_data_ = std::move(extra_data);
  }
  void SetRawDeltaSink(RawDeltaSink&& raw_delta) {
    raw_delta_ = std::move(raw_delta);
  }
  void SetReferenceDeltaSink(ReferenceDeltaSink reference_delta) {
    reference_delta_ = std::move(reference_delta);
  }
  // Set additional targets for pool identified with |pool_tag|.
  void SetTargetSink(PoolTag pool_tag, TargetSink&& extra_targets) {
    DCHECK(pool_tag != kNoPoolTag);
    extra_targets_[pool_tag] = std::move(extra_targets);
  }

  // Returns the serialized size in bytes of the data this object is holding.
  size_t SerializedSize() const;

  // If sufficient space is available, serializes data into |sink|, which is at
  // least SerializedSize() bytes and returns true. Otherwise returns false.
  bool SerializeInto(BufferSink* sink) const;

 private:
  ElementMatch element_match_;
  EquivalenceSink equivalences_;
  ExtraDataSink extra_data_;
  RawDeltaSink raw_delta_;
  ReferenceDeltaSink reference_delta_;
  std::map<PoolTag, TargetSink> extra_targets_;
};

// Utility to write a Zucchini ensemble patch. An ensemble patch is the
// concatenation of a patch header with a vector of patch elements.
class EnsemblePatchSink {
 public:
  explicit EnsemblePatchSink(const PatchHeader& header);
  explicit EnsemblePatchSink(ConstBufferView old_image,
                             ConstBufferView new_image);
  ~EnsemblePatchSink();

  void SetPatchType(PatchType patch_type) { patch_type_ = patch_type; }

  // Adds an element into the patch.
  void AddElement(PatchElementSink&& element_patch);

  // Returns the serialized size in bytes of the data this object is holding.
  size_t SerializedSize() const;

  // If sufficient space is available, serializes data into |sink|, which is at
  // least SerializedSize() bytes and returns true. Otherwise returns false.
  bool SerializeInto(BufferSink* sink) const;

  // If sufficient space is available, serializes data into |buffer|, which is
  // at least SerializedSize() bytes and returns true. Otherwise returns false.
  bool SerializeInto(MutableBufferView buffer) const {
    BufferSink sink(buffer);
    return SerializeInto(&sink);
  }

 private:
  PatchHeader header_;
  PatchType patch_type_ = PatchType::kUnrecognisedPatch;
  std::vector<PatchElementSink> elements_;
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_PATCH_SINK_H_
