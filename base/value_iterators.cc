// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/value_iterators.h"

namespace base {

namespace detail {

// ----------------------------------------------------------------------------
// dict_iterator.

inline dict_iterator::pointer::pointer(const reference& ref) : ref_(ref) {}

inline dict_iterator::pointer::pointer(const pointer& ptr) : ref_(ptr.ref_) {}

inline dict_iterator::pointer& dict_iterator::pointer::operator=(
    const pointer& ptr) {
  ref_ = ptr.ref_;
  return *this;
}

inline dict_iterator::dict_iterator(DictStorage::iterator dict_iter)
    : dict_iter_(dict_iter) {}

inline dict_iterator::dict_iterator(const dict_iterator& dict_iter) {
  dict_iter_ = dict_iter.dict_iter_;
}

inline dict_iterator& dict_iterator::operator=(const dict_iterator& dict_iter) {
  dict_iter_ = dict_iter.dict_iter_;
  return *this;
}

inline dict_iterator::reference dict_iterator::operator*() const {
  return {std::ref(dict_iter_->first), std::ref(*dict_iter_->second)};
}

inline dict_iterator::pointer dict_iterator::operator->() const {
  return pointer(operator*());
}

inline dict_iterator& dict_iterator::operator++() {
  ++dict_iter_;
  return *this;
}

inline dict_iterator dict_iterator::operator++(int) {
  dict_iterator tmp(*this);
  ++dict_iter_;
  return tmp;
}

inline dict_iterator& dict_iterator::operator--() {
  --dict_iter_;
  return *this;
}

inline dict_iterator dict_iterator::operator--(int) {
  dict_iterator tmp(*this);
  --dict_iter_;
  return tmp;
}

inline bool operator==(const dict_iterator& lhs, const dict_iterator& rhs) {
  return lhs.dict_iter_ == rhs.dict_iter_;
}

// ----------------------------------------------------------------------------
// const_dict_iterator.

inline const_dict_iterator::pointer::pointer(const reference& ref)
    : ref_(ref) {}

inline const_dict_iterator::pointer::pointer(const pointer& ptr)
    : ref_(ptr.ref_) {}

inline const_dict_iterator::pointer& const_dict_iterator::pointer::operator=(
    const pointer& ptr) {
  ref_ = ptr.ref_;
  return *this;
}

inline const_dict_iterator::const_dict_iterator(
    DictStorage::const_iterator dict_iter)
    : dict_iter_(dict_iter) {}

inline const_dict_iterator::const_dict_iterator(
    const const_dict_iterator& dict_iter) {
  dict_iter_ = dict_iter.dict_iter_;
}

inline const_dict_iterator& const_dict_iterator::operator=(
    const const_dict_iterator& dict_iter) {
  dict_iter_ = dict_iter.dict_iter_;
  return *this;
}

inline const_dict_iterator::reference const_dict_iterator::operator*() const {
  return {std::cref(dict_iter_->first), std::cref(*dict_iter_->second)};
}

inline const_dict_iterator::pointer const_dict_iterator::operator->() const {
  return pointer(operator*());
}

inline const_dict_iterator& const_dict_iterator::operator++() {
  ++dict_iter_;
  return *this;
}

inline const_dict_iterator const_dict_iterator::operator++(int) {
  const_dict_iterator tmp(*this);
  ++dict_iter_;
  return tmp;
}

inline const_dict_iterator& const_dict_iterator::operator--() {
  --dict_iter_;
  return *this;
}

inline const_dict_iterator const_dict_iterator::operator--(int) {
  const_dict_iterator tmp(*this);
  --dict_iter_;
  return tmp;
}

inline bool operator==(const const_dict_iterator& lhs,
                       const const_dict_iterator& rhs) {
  return lhs.dict_iter_ == rhs.dict_iter_;
}

// ----------------------------------------------------------------------------
// dict_iterator_proxy.

inline dict_iterator_proxy::dict_iterator_proxy(DictStorage* storage)
    : storage_(storage) {}

inline dict_iterator dict_iterator_proxy::begin() {
  return dict_iterator(storage_->begin());
}

inline const_dict_iterator dict_iterator_proxy::begin() const {
  return const_dict_iterator(storage_->begin());
}

inline dict_iterator dict_iterator_proxy::end() {
  return dict_iterator(storage_->end());
}

inline const_dict_iterator dict_iterator_proxy::end() const {
  return const_dict_iterator(storage_->end());
}

inline reverse_dict_iterator dict_iterator_proxy::rbegin() {
  return reverse_dict_iterator(end());
}

inline const_reverse_dict_iterator dict_iterator_proxy::rbegin() const {
  return const_reverse_dict_iterator(end());
}

inline reverse_dict_iterator dict_iterator_proxy::rend() {
  return reverse_dict_iterator(begin());
}

inline const_reverse_dict_iterator dict_iterator_proxy::rend() const {
  return const_reverse_dict_iterator(begin());
}

inline const_dict_iterator dict_iterator_proxy::cbegin() const {
  return const_dict_iterator(begin());
}

inline const_dict_iterator dict_iterator_proxy::cend() const {
  return const_dict_iterator(end());
}

inline const_reverse_dict_iterator dict_iterator_proxy::crbegin() const {
  return const_reverse_dict_iterator(rbegin());
}

inline const_reverse_dict_iterator dict_iterator_proxy::crend() const {
  return const_reverse_dict_iterator(rend());
}

// ----------------------------------------------------------------------------
// const_dict_iterator_proxy.

inline const_dict_iterator const_dict_iterator_proxy::begin() const {
  return const_dict_iterator(storage_->begin());
}

inline const_dict_iterator const_dict_iterator_proxy::end() const {
  return const_dict_iterator(storage_->end());
}

inline const_reverse_dict_iterator const_dict_iterator_proxy::rbegin() const {
  return const_reverse_dict_iterator(end());
}

inline const_reverse_dict_iterator const_dict_iterator_proxy::rend() const {
  return const_reverse_dict_iterator(begin());
}

inline const_dict_iterator const_dict_iterator_proxy::cbegin() const {
  return const_dict_iterator(begin());
}

inline const_dict_iterator const_dict_iterator_proxy::cend() const {
  return const_dict_iterator(end());
}

inline const_reverse_dict_iterator const_dict_iterator_proxy::crbegin() const {
  return const_reverse_dict_iterator(rbegin());
}

inline const_reverse_dict_iterator const_dict_iterator_proxy::crend() const {
  return const_reverse_dict_iterator(rend());
}

}  // namespace detail

}  // namespace base
