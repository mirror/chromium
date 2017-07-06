// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_SINK_PATCH_H_
#define CHROME_INSTALLER_ZUCCHINI_SINK_PATCH_H_

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

bool SerializeInto(ElementMatch element_match, BufferSink* sink);
size_t SerializedSize(ElementMatch element_match);

bool SerializeInto(const std::vector<uint8_t>& buffer, BufferSink* sink);
size_t SerializedSize(const std::vector<uint8_t>& buffer);

//  Receives equivalences and serialize them into underlying buffers.
class EquivalenceSink {
 public:
  EquivalenceSink();
  EquivalenceSink(const std::vector<uint8_t>& src_skip,
                  const std::vector<uint8_t>& dst_skip,
                  const std::vector<uint8_t>& copy_count);

  EquivalenceSink(EquivalenceSink&&);
  ~EquivalenceSink();

  EquivalenceSink& operator=(EquivalenceSink&&) = default;

  size_t SerializedSize() const;
  bool SerializeInto(BufferSink* buffer) const;

  void PutNext(const Equivalence& equivalence);

 private:
  std::vector<uint8_t> src_skip_;
  std::vector<uint8_t> dst_skip_;
  std::vector<uint8_t> copy_count_;

  offset_t src_offset_ = 0;
  offset_t dst_offset_ = 0;
};

// Receives extra data and serialize it into underlying buffers.
class ExtraDataSink {
 public:
  ExtraDataSink();
  ExtraDataSink(const std::vector<uint8_t>& extra_data);
  ExtraDataSink(ExtraDataSink&&);
  ~ExtraDataSink();

  ExtraDataSink& operator=(ExtraDataSink&&) = default;

  void PutNext(BufferView region);

  size_t SerializedSize() const;
  bool SerializeInto(BufferSink* buffer) const;

 private:
  std::vector<uint8_t> extra_data_;
};

// Receives raw deltas and serialize them into underlying buffers.
class RawDeltaSink {
 public:
  RawDeltaSink();
  RawDeltaSink(const std::vector<uint8_t>& raw_delta_skip,
               const std::vector<uint8_t>& raw_delta_diff);
  RawDeltaSink(RawDeltaSink&&);
  ~RawDeltaSink();

  RawDeltaSink& operator=(RawDeltaSink&&) = default;

  void PutNext(RawDeltaUnit delta);

  size_t SerializedSize() const;
  bool SerializeInto(BufferSink* buffer) const;

 private:
  std::vector<uint8_t> raw_delta_skip_;
  std::vector<uint8_t> raw_delta_diff_;
};

// Receives reference deltas and serialize them into underlying buffers.
class ReferenceDeltaSink {
 public:
  ReferenceDeltaSink();
  ReferenceDeltaSink(const std::vector<uint8_t>& reference_delta);
  ReferenceDeltaSink(ReferenceDeltaSink&&);
  ~ReferenceDeltaSink();

  ReferenceDeltaSink& operator=(ReferenceDeltaSink&&) = default;

  void PutNext(int32_t diff);

  size_t SerializedSize() const;
  bool SerializeInto(BufferSink* buffer) const;

 private:
  std::vector<uint8_t> reference_delta_;
};

// Receives symbols and serialize them into underlying buffers.
class TargetSink {
 public:
  TargetSink();
  TargetSink(const std::vector<uint8_t>& extra_targets);
  TargetSink(TargetSink&&);
  ~TargetSink();

  TargetSink& operator=(TargetSink&&) = default;

  void PutNext(uint32_t symbol);

  size_t SerializedSize() const;
  bool SerializeInto(BufferSink* buffer) const;

 private:
  std::vector<uint8_t> extra_targets_;

  offset_t previous_target_ = 0;
};

class ElementPatchSink {
 public:
  ElementPatchSink();
  ElementPatchSink(ElementMatch element_match);
  ElementPatchSink(ElementPatchSink&&);
  ~ElementPatchSink();
  ElementPatchSink& operator=(ElementPatchSink&&) = default;

  size_t SerializedSize() const;
  bool SerializeInto(BufferSink* buffer) const;

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
  void SetTargetSink(PoolTag pool_tag, TargetSink&& extra_targets) {
    DCHECK(pool_tag != kNoPoolTag);
    extra_targets_.emplace(pool_tag, std::move(extra_targets));
  }

 private:
  ElementMatch element_match_;
  EquivalenceSink equivalences_;
  ExtraDataSink extra_data_;
  RawDeltaSink raw_delta_;
  ReferenceDeltaSink reference_delta_;
  std::map<PoolTag, TargetSink> extra_targets_;
};

// Utility to write a Zucchini patch, and outer class for Sink classes
// that manage stream access.
class EnsemblePatchSink {
 public:
  EnsemblePatchSink(BufferView old_image, BufferView new_image);
  ~EnsemblePatchSink();

  size_t SerializedSize() const;
  bool SerializeInto(BufferSink* buffer) const;
  bool SerializeInto(MutableBufferView buffer) const {
    BufferSink sink(buffer);
    return SerializeInto(&sink);
  }

  void AddElement(ElementPatchSink&& element_patch) {
    elements_.push_back(std::move(element_patch));
  }

 private:
  PatchHeader header_;
  std::vector<ElementPatchSink> elements_;
};

}  // namespace zucchini

#endif  // ZUCCHINI_PATCH_H_
