// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/dom_storage/dom_storage_map.h"

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"

namespace content {

namespace {

size_t size_of_item(const base::string16& key,
                    const base::string16& value,
                    bool has_only_keys) {
  if (!has_only_keys)
    return (key.length() + value.length()) * sizeof(base::char16);
  size_t value_size;
  base::StringToSizeT(value, &value_size);
  return key.length() * sizeof(base::char16) + value_size;
}

}  // namespace

DOMStorageMap::DOMStorageMap(size_t quota)
    : bytes_used_(0), memory_usage_(0), quota_(quota), has_only_keys_(false) {
  ResetKeyIterator();
}

DOMStorageMap::DOMStorageMap(size_t quota, bool has_only_keys)
    : bytes_used_(0),
      memory_usage_(0),
      quota_(quota),
      has_only_keys_(has_only_keys) {}

DOMStorageMap::~DOMStorageMap() {}

unsigned DOMStorageMap::Length() const {
  return values_.size();
}

base::NullableString16 DOMStorageMap::Key(unsigned index) {
  if (index >= values_.size())
    return base::NullableString16();
  while (last_key_index_ != index) {
    if (last_key_index_ > index) {
      --key_iterator_;
      --last_key_index_;
    } else {
      ++key_iterator_;
      ++last_key_index_;
    }
  }
  return base::NullableString16(key_iterator_->first, false);
}

bool DOMStorageMap::SetItem(const base::string16& key,
                            const base::string16& value) {
  base::NullableString16 old_value;
  return SetItemInternal(key, value, &old_value);
}

bool DOMStorageMap::SetItem(
    const base::string16& key, const base::string16& value,
    base::NullableString16* old_value) {
  DCHECK(!has_only_keys_);
  return SetItemInternal(key, value, old_value);
}

bool DOMStorageMap::SetItemInternal(const base::string16& key,
                                    const base::string16& value,
                                    base::NullableString16* old_value) {
  DOMStorageValuesMap::const_iterator found = values_.find(key);
  if (found == values_.end())
    *old_value = base::NullableString16();
  else
    *old_value = found->second;

  size_t old_item_size =
      old_value->is_null()
          ? 0
          : size_of_item(key, old_value->string(), has_only_keys_);
  size_t new_item_size = size_of_item(key, value, false);
  size_t new_bytes_used = bytes_used_ - old_item_size + new_item_size;

  // Only check quota if the size is increasing, this allows
  // shrinking changes to pre-existing files that are over budget.
  if (new_item_size > old_item_size && new_bytes_used > quota_)
    return false;

  base::NullableString16 new_value;
  if (has_only_keys_)
    new_value = base::NullableString16(
        base::SizeTToString16(value.length() * sizeof(base::char16)), false);
  else
    new_value = base::NullableString16(value, false);
  values_[key] = new_value;
  ResetKeyIterator();
  bytes_used_ = new_bytes_used;
  size_t old_item_memory =
      old_value->is_null() ? 0 : size_of_item(key, old_value->string(), false);
  memory_usage_ = memory_usage_ + size_of_item(key, new_value.string(), false) -
                  old_item_memory;
  return true;
}

bool DOMStorageMap::RemoveItem(const base::string16& key) {
  base::string16 old_value;
  return RemoveItemInternal(key, &old_value);
}

bool DOMStorageMap::RemoveItem(
    const base::string16& key,
    base::string16* old_value) {
  DCHECK(!has_only_keys_);
  return RemoveItemInternal(key, old_value);
}

bool DOMStorageMap::RemoveItemInternal(const base::string16& key,
                                       base::string16* old_value) {
  DOMStorageValuesMap::iterator found = values_.find(key);
  if (found == values_.end())
    return false;
  *old_value = found->second.string();
  values_.erase(found);
  ResetKeyIterator();
  bytes_used_ -= size_of_item(key, *old_value, has_only_keys_);
  memory_usage_ -= size_of_item(key, *old_value, false);
  return true;
}

base::NullableString16 DOMStorageMap::GetItem(const base::string16& key) const {
  DCHECK(!has_only_keys_);
  DOMStorageValuesMap::const_iterator found = values_.find(key);
  if (found == values_.end())
    return base::NullableString16();
  return found->second;
}

void DOMStorageMap::TakeValuesFrom(DOMStorageValuesMap* values) {
  // Note: A pre-existing file may be over the quota budget.
  if (has_only_keys_) {
    values_.clear();
    // TODO(ssid): This could be optimized if the storage read the only the
    // keys.
    for (const auto& item : *values) {
      values_[item.first] = base::NullableString16(
          base::SizeTToString16(item.second.string().length() *
                                sizeof(base::char16)),
          false);
    }
  } else {
    values_.swap(*values);
  }
  bytes_used_ = CountBytes(values_, has_only_keys_);
  memory_usage_ = CountBytes(values_, false);
  ResetKeyIterator();
}

DOMStorageMap* DOMStorageMap::DeepCopy() const {
  DOMStorageMap* copy = new DOMStorageMap(quota_, has_only_keys_);
  copy->values_ = values_;
  copy->bytes_used_ = bytes_used_;
  copy->memory_usage_ = memory_usage_;
  copy->ResetKeyIterator();
  return copy;
}

void DOMStorageMap::ResetKeyIterator() {
  key_iterator_ = values_.begin();
  last_key_index_ = 0;
}

size_t DOMStorageMap::CountBytes(const DOMStorageValuesMap& values,
                                 bool has_only_keys) {
  if (values.empty())
    return 0;

  size_t count = 0;
  for (const auto& pair : values)
    count += size_of_item(pair.first, pair.second.string(), has_only_keys);
  return count;
}

}  // namespace content
