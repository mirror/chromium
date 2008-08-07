// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#ifndef V8_LIST_INL_H_
#define V8_LIST_INL_H_

#include "list.h"

namespace v8 { namespace internal {


template<typename T, class P>
T& List<T, P>::Add(const T& element) {
  if (length_ >= capacity_) {
    // Grow the list capacity by 50%, but make sure to let it grow
    // even when the capacity is zero (possible initial case).
    int new_capacity = 1 + capacity_ + (capacity_ >> 1);
    T* new_data = NewData(new_capacity);
    memcpy(new_data, data_, capacity_ * sizeof(T));
    DeleteData(data_);
    data_ = new_data;
    capacity_ = new_capacity;
  }
  return data_[length_++] = element;
}


template<typename T, class P>
Vector<T> List<T, P>::AddBlock(const T& element, int count) {
  int start = length_;
  for (int i = 0; i < count; i++)
    Add(element);
  return Vector<T>(&data_[start], count);
}


template<typename T, class P>
T List<T, P>::Remove(int i) {
  T element = at(i);
  length_--;
  while (i < length_) {
    data_[i] = data_[i + 1];
    i++;
  }
  return element;
}


template<typename T, class P>
void List<T, P>::Clear() {
  DeleteData(data_);
  Initialize(0);
}


template<typename T, class P>
void List<T, P>::Rewind(int pos) {
  length_ = pos;
}


template<typename T, class P>
void List<T, P>::Iterate(void (*callback)(T* x)) {
  for (int i = 0; i < length_; i++) callback(&data_[i]);
}


template<typename T, class P>
void List<T, P>::Sort(int (*cmp)(const T* x, const T* y)) {
  qsort(data_,
        length_,
        sizeof(T),
        reinterpret_cast<int (*)(const void*, const void*)>(cmp));
#ifdef DEBUG
  for (int i = 1; i < length_; i++)
    ASSERT(cmp(&data_[i - 1], &data_[i]) <= 0);
#endif
}


template<typename T, class P>
void List<T, P>::Initialize(int capacity) {
  ASSERT(capacity >= 0);
  data_ = (capacity > 0) ? NewData(capacity) : NULL;
  capacity_ = capacity;
  length_ = 0;
}


} }  // namespace v8::internal

#endif  // V8_LIST_INL_H_
