// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_PATCH_WRITER_H_
#define CHROME_INSTALLER_ZUCCHINI_PATCH_WRITER_H_

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "chrome/installer/zucchini/buffer_sink.h"
#include "chrome/installer/zucchini/buffer_view.h"
#include "chrome/installer/zucchini/image_utils.h"
#include "chrome/installer/zucchini/patch_utils.h"

namespace zucchini {

namespace patch {

// If sufficient space is available, serializes |element_match| into |sink| and
// returns true. Otherwise returns false.
bool SerializeElementMatch(const ElementMatch& element_match, BufferSink* sink);

// Returns the size in bytes required to serialize |element_match|.
size_t SerializedElementMatchSize(const ElementMatch& element_match);

// If sufficient space is available, serializes |buffer| into |sink| and returns
// true. Otherwise returns false.
bool SerializeBuffer(const std::vector<uint8_t>& buffer, BufferSink* sink);

// Returns the size in bytes required to serialize |buffer|.
size_t SerializedBufferSize(const std::vector<uint8_t>& buffer);

}  // namespace patch

// Following classes behave like output streams, providing write capability of
// structured data with the method PutNext(). These write into internal buffers
// that can then be serialized with the method SerializeInto(buffer) to a buffer
// large enough to hold SerializedSize() bytes. Some sinks internally use
// several buffers to simultenaously write into multiple channels. These buffer
// are then concatenated upon SerializeInto(). This way, writing data only
// requires one pass and computing the serialized size is easy.

// Sink for equivalences.
class EquivalenceSink {
 public:
  EquivalenceSink();
  EquivalenceSink(const std::vector<uint8_t>& src_skip,
                  const std::vector<uint8_t>& dst_skip,
                  const std::vector<uint8_t>& copy_count);

  EquivalenceSink(EquivalenceSink&&);
  ~EquivalenceSink();

  // Writes the next |equivalence| into the object. Equivalence are expected to
  // be given ordered by |Equivalence::dst_offset|.
  void PutNext(const Equivalence& equivalence);

  // Returns the serialized size in bytes of the data this object is holding.
  size_t SerializedSize() const;

  // If sufficient space is available, serializes data into |sink|, which is at
  // least SerializedSize() bytes, and returns true. Otherwise returns false.
  bool SerializeInto(BufferSink* buffer) const;

 private:
  // Offset in source, delta-encoded starting from end of last equivalence, and
  // stored as signed varint.
  std::vector<uint8_t> src_skip_;
  // Offset in destination, delta-encoded starting from end of last equivalence,
  // and stored as unsigned varint.
  std::vector<uint8_t> dst_skip_;
  // Length of equivalence stored as unsigned varint.
  // TODO(etiennep): Investigate on bias.
  std::vector<uint8_t> copy_count_;

  offset_t src_offset_ = 0;  // Last offset in source.
  offset_t dst_offset_ = 0;  // Last offset in destination.
};

// Sink for extra data.
class ExtraDataSink {
 public:
  ExtraDataSink();
  explicit ExtraDataSink(const std::vector<uint8_t>& extra_data);
  ExtraDataSink(ExtraDataSink&&);
  ~ExtraDataSink();

  // Writes bytes from |region| into the object.
  void PutNext(ConstBufferView region);

  // Returns the serialized size in bytes of the data this object is holding.
  size_t SerializedSize() const;

  // If sufficient space is available, serializes data into |sink|, which is at
  // least SerializedSize() bytes, and returns true. Otherwise returns false.
  bool SerializeInto(BufferSink* sink) const;

 private:
  std::vector<uint8_t> extra_data_;
};

// Sink for raw delta.
class RawDeltaSink {
 public:
  RawDeltaSink();
  RawDeltaSink(const std::vector<uint8_t>& raw_delta_skip,
               const std::vector<uint8_t>& raw_delta_diff);
  RawDeltaSink(RawDeltaSink&&);
  ~RawDeltaSink();

  // Writes the next |delta| into the object. Deltas are expected to be given
  // ordered by |copy_offset|.
  void PutNext(const RawDeltaUnit& delta);

  // Returns the serialized size in bytes of the data this object is holding.
  size_t SerializedSize() const;

  // If sufficient space is available, serializes data into |sink|, which is at
  // least SerializedSize() bytes, and returns true. Otherwise returns false.
  bool SerializeInto(BufferSink* sink) const;

 private:
  std::vector<uint8_t> raw_delta_skip_;  // Copy offset stating from last delta.
  std::vector<uint8_t> raw_delta_diff_;  // Bytewise difference.

  offset_t copy_offset_compensation_ = 0;
};

// Sink for reference delta.
class ReferenceDeltaSink {
 public:
  ReferenceDeltaSink();
  explicit ReferenceDeltaSink(const std::vector<uint8_t>& reference_delta);
  ReferenceDeltaSink(ReferenceDeltaSink&&);
  ~ReferenceDeltaSink();

  // Writes the next |diff| into the object.
  void PutNext(int32_t diff);

  // Returns the serialized size in bytes of the data this object is holding.
  size_t SerializedSize() const;

  // If sufficient space is available, serializes data into |sink|, which is at
  // least SerializedSize() bytes, and returns true. Otherwise returns false.
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

  // Writes the next |target| into the object. Targets are expected to be given
  // in acending order.
  void PutNext(uint32_t target);

  // Returns the serialized size in bytes of the data this object is holding.
  size_t SerializedSize() const;

  // If sufficient space is available, serializes data into |sink|, which is at
  // least SerializedSize() bytes, and returns true. Otherwise returns false.
  bool SerializeInto(BufferSink* sink) const;

 private:
  // Targets are delta-encoded and biaised by 1, stored as unsigned varint.
  std::vector<uint8_t> extra_targets_;

  offset_t target_compensation_ = 0;
};

// Following are utility classes to write structured data forming a patch.

// Utility to write a patch element. A patch element contains all the
// information necessary to patch a single element. This class provides an
// interface to individually set different building blocks of data in the patch
// element.
class PatchElementWriter {
 public:
  PatchElementWriter();
  explicit PatchElementWriter(ElementMatch element_match);
  PatchElementWriter(PatchElementWriter&&);
  ~PatchElementWriter();

  // Following methods set individual blocks for this element. Previous
  // corresponding block is replaced. All streams must be set before call to
  // SerializedSize() of SerializeInto().

  void SetEquivalenceSink(EquivalenceSink&& equivalences) {
    equivalences_.emplace(std::move(equivalences));
  }
  void SetExtraDataSink(ExtraDataSink&& extra_data) {
    extra_data_.emplace(std::move(extra_data));
  }
  void SetRawDeltaSink(RawDeltaSink&& raw_delta) {
    raw_delta_.emplace(std::move(raw_delta));
  }
  void SetReferenceDeltaSink(ReferenceDeltaSink reference_delta) {
    reference_delta_.emplace(std::move(reference_delta));
  }
  // Set additional targets for pool identified with |pool_tag|.
  void SetTargetSink(PoolTag pool_tag, TargetSink&& extra_targets) {
    DCHECK(pool_tag != kNoPoolTag);
    extra_targets_.emplace(pool_tag, std::move(extra_targets));
  }

  // Returns the serialized size in bytes of the data this object is holding.
  size_t SerializedSize() const;

  // If sufficient space is available, serializes data into |sink|, which is at
  // least SerializedSize() bytes, and returns true. Otherwise returns false.
  bool SerializeInto(BufferSink* sink) const;

 private:
  ElementMatch element_match_;
  base::Optional<EquivalenceSink> equivalences_;
  base::Optional<ExtraDataSink> extra_data_;
  base::Optional<RawDeltaSink> raw_delta_;
  base::Optional<ReferenceDeltaSink> reference_delta_;
  std::map<PoolTag, TargetSink> extra_targets_;
};

// Utility to write a Zucchini ensemble patch. An ensemble patch is the
// concatenation of a patch header with a vector of patch elements.
class EnsemblePatchWriter {
 public:
  explicit EnsemblePatchWriter(const PatchHeader& header);
  EnsemblePatchWriter(ConstBufferView old_image, ConstBufferView new_image);
  ~EnsemblePatchWriter();

  void SetPatchType(PatchType patch_type) { patch_type_ = patch_type; }

  // Adds an element into the patch.
  void AddElement(PatchElementWriter&& element_patch);

  // Returns the serialized size in bytes of the data this object is holding.
  size_t SerializedSize() const;

  // If sufficient space is available, serializes data into |sink|, which is at
  // least SerializedSize() bytes, and returns true. Otherwise returns false.
  bool SerializeInto(BufferSink* sink) const;

  // If sufficient space is available, serializes data into |buffer|, which is
  // at least SerializedSize() bytes, and returns true. Otherwise returns false.
  bool SerializeInto(MutableBufferView buffer) const {
    BufferSink sink(buffer);
    return SerializeInto(&sink);
  }

 private:
  PatchHeader header_;
  PatchType patch_type_ = PatchType::kUnrecognisedPatch;
  std::vector<PatchElementWriter> elements_;

  DISALLOW_COPY_AND_ASSIGN(EnsemblePatchWriter);
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_PATCH_WRITER_H_
