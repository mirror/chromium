// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_IMAGE_INDEX_H_
#define CHROME_INSTALLER_ZUCCHINI_IMAGE_INDEX_H_

#include <stdint.h>

#include <vector>

#include "base/logging.h"
#include "base/optional.h"
#include "chrome/installer/zucchini/buffer_view.h"
#include "chrome/installer/zucchini/image_utils.h"

namespace zucchini {

class ReferenceReader;

// A class that holds annotations of an image file, allowing quick access to its
// raw and reference content.
class ImageIndex {
 public:
  explicit ImageIndex(ConstBufferView image);
  ~ImageIndex();

  // Inserts all references read from |ref_reader| to this index. |traits|
  // describes references being inserted. This should be called exactly once for
  // each reference type. If overlap bewteen any two references of any type is
  // encountered, returns false and leaves the object in an invalid state.
  // Otherwise, returns true.
  bool InsertReferences(const ReferenceTypeTraits& traits,
                        ReferenceReader&& ref_reader);

  // Returns the number of reference type the index holds.
  size_t TypeCount() const { return references_.size(); }

  // Returns the number of target pool discovered.
  size_t PoolCount() const { return pool_count_; }

  // Returns traits describing references of type |type|.
  const ReferenceTypeTraits& GetTraits(TypeTag type) const {
    DCHECK_LT(type.value(), traits_.size());
    return traits_[type.value()];
  }

  // Returns true if content at |location| is either:
  // - A raw value.
  // - The first byte of a reference.
  bool IsToken(offset_t location) const;

  // Returns true if content at |location| is part of a reference.
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

  // Returns the reference of type |type| covering |location|, or base::nullopt
  // if |location| is not part of a reference of type |type|.
  base::Optional<Reference> FindReference(TypeTag type,
                                          offset_t location) const;

  // Returns a vector of references of type |type|, where references are sorted
  // by their location.
  const std::vector<Reference>& GetReferences(TypeTag type) const;

  ConstBufferView raw_image() const { return raw_image_; }

  // Returns the size of the image.
  size_t size() const { return raw_image_.size(); }

 private:
  ConstBufferView raw_image_;

  // Used for random access lookup of reference type.
  std::vector<TypeTag> type_tags_;

  // For a given type, |references_[type]| stores a list of Reference instances
  // sorted by location (file offset).
  std::vector<std::vector<Reference>> references_;
  std::vector<ReferenceTypeTraits> traits_;

  uint8_t pool_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(ImageIndex);
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_IMAGE_INDEX_H_
