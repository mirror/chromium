// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_IMAGE_INDEX_H_
#define CHROME_INSTALLER_ZUCCHINI_IMAGE_INDEX_H_

#include <stdint.h>

#include <vector>

#include "base/containers/flat_set.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/optional.h"
#include "chrome/installer/zucchini/buffer_view.h"
#include "chrome/installer/zucchini/image_utils.h"

namespace zucchini {

class Disassembler;
class EquivalenceMap;
class TargetSource;

class TargetPool {
 public:
  TargetPool();
  TargetPool(TargetPool&&);
  TargetPool(const TargetPool&);
  ~TargetPool();

  // Inserts every new targets found in references. This invalidates all
  // previous Index lookups. Labels are not assigned automatically and need to
  // be set with |ResetLabels|. Once labels are assigned, no additional targets
  // can be inserted.
  void InsertTargets(const std::vector<Reference>& references);
  void InsertTargets(ReferenceReader&& reader);
  void InsertTargets(TargetSource* targets);
  void InsertTargets(const std::vector<offset_t>& targets);
  
  void AddType(TypeTag type) {
    types_.push_back(type);
  }
  
  void Project(const EquivalenceMap& equivalence_map);

  // Returns true if |target| is found in targets.
  bool ContainsTarget(offset_t target) const;

  // Returns the index at which |target| is stored, or nullopt if |target| is
  // not found.
  offset_t FindKeyForOffset(offset_t target) const {
    auto pos = std::lower_bound(targets_.begin(), targets_.end(), target);
    return static_cast<offset_t>(pos - targets_.begin());
  }

  // Returns the target for a given valid |index|.
  offset_t GetOffsetForKey(offset_t key) const {
    return targets_[key];
  }

  // Returns the number of targets.
  size_t size() const { return targets_.size(); }

  // Returns the ordered targets vector.
  const std::vector<offset_t>& targets() const { return targets_; }
  const std::vector<TypeTag>& types() const { return types_; }

 private:
  std::vector<offset_t> targets_;
  std::vector<TypeTag> types_;  // Enumerates type_tag for this pool.
};

class ReferenceSet {
 public:
  ReferenceSet(const ReferenceTypeTraits& traits);
  ReferenceSet(ReferenceSet&&);
  ~ReferenceSet();
 
  void InsertReferences(ReferenceReader&& ref_reader);
  void CanonizeTargets(const TargetPool& target_pool);
  
  const std::vector<Reference>& references() const { return references_; }
  const ReferenceTypeTraits& traits() const { return traits_; }
  TypeTag type_tag() const { return traits_.type_tag; }
  PoolTag pool_tag() const { return traits_.pool_tag; }
  offset_t width() const { return traits_.width; }
  
  Reference FindReference(offset_t location) const;
 
 private:
  ReferenceTypeTraits traits_;
  // List of Reference instances sorted by location (file offset).
  std::vector<Reference> references_;
  
  DISALLOW_COPY_AND_ASSIGN(ReferenceSet);
};

// A class that holds annotations of an image, allowing quick access to its
// raw and reference content. The memory overhead of storing all references is
// relatively high, so this is only used during patch generation.
class ImageIndex {
 public:
  ImageIndex(ConstBufferView image);
  ~ImageIndex();

  // Inserts all references of type |type_tag| read from |ref_reader| to this
  // index. This should be called exactly once for each reference type. If
  // overlap between any two references of any type is encountered, returns
  // false and leaves the object in an invalid state. Otherwise, returns true.
  bool InsertReferences(Disassembler* disassembler);

  // Returns the number of reference types the index holds.
  size_t TypeCount() const { return types_.size(); }

  // Returns the number of target pools discovered.
  size_t PoolCount() const { return pools_.size(); }

  // Returns traits describing references of type |type|.
  const ReferenceTypeTraits& GetTraits(TypeTag type) const {
    DCHECK_LT(type.value(), types_.size());
    return types_[type.value()].traits();
  }

  PoolTag GetPoolTag(TypeTag type) const { return GetTraits(type).pool_tag; }

  const std::vector<TypeTag>& GetTypeTags(PoolTag pool) const {
    DCHECK_LT(pool.value(), pools_.size());
    return pools_[pool.value()].types();
  }

  // Returns true if |raw_image_[location]| is either:
  // - A raw value.
  // - The first byte of a reference.
  bool IsToken(offset_t location) const;

  // Returns true if |raw_image_[location]| is part of a reference.
  bool IsReference(offset_t location) const {
    return GetType(location) != kNoTypeTag;
  }

  // Returns the type tag of the reference covering |location|, or kNoTypeTag if
  // |location| is not part of a reference.
  TypeTag GetType(offset_t location) const {
    DCHECK_EQ(type_tags_.size(), raw_image_.size());  // Sanity check.
    DCHECK_LT(location, size());
    return type_tags_[location];
  }

  // Returns the raw value at |location|.
  uint8_t GetRawValue(offset_t location) const {
    DCHECK_LT(location, size());
    return raw_image_[location];
  }

  // Returns the reference of type |type| covering |location|, which is expected
  // to exist.
  Reference FindReference(TypeTag type, offset_t location) const;
  
  /*offset_t GetTargetOffsetForKey(PoolTag pool, offset_t key) const {
    DCHECK_LT(pool.value(), pools_.size());
    return pools_[pool.value()].GetOffsetForKey(key);
  }*/

  // Returns a vector of references of type |type|, where references are sorted
  // by their location.
  const std::vector<Reference>& GetReferences(TypeTag type) const {
    DCHECK_LT(type.value(), types_.size());
    return types_[type.value()].references();
  }

  // Returns a vector of all targets in |pool|, where targets are sorted.
  const TargetPool& GetTargetPool(PoolTag pool) const {
    DCHECK_LT(pool.value(), pools_.size());
    return pools_[pool.value()];
  }

  // Returns the size of the image.
  size_t size() const { return raw_image_.size(); }

 private:
  const ConstBufferView raw_image_;

  // Used for random access lookup of reference type, for each byte in
  // |raw_image_|.
  std::vector<TypeTag> type_tags_;

  std::vector<ReferenceSet> types_;
  std::vector<TargetPool> pools_;
  
  DISALLOW_COPY_AND_ASSIGN(ImageIndex);
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_IMAGE_INDEX_H_
