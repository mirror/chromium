// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_STREAM_H_
#define ZUCCHINI_STREAM_H_

#include <algorithm>
#include <cstdint>
#include <deque>
#include <iterator>
#include <vector>

#include "zucchini/ranges/algorithm.h"
#include "zucchini/region.h"

namespace zucchini {

// Manages an iterator that counts number of bytes written.
class SizeMeasurer {
 public:
  class iterator
      : public std::iterator<std::output_iterator_tag, void, void, void, void> {
   public:
    explicit iterator(size_t& count) : count_(&count) {}

    iterator& operator=(uint8_t value) {
      ++(*count_);
      return *this;
    }

    iterator& operator*() { return *this; }

    iterator& operator++() { return *this; }
    iterator& operator++(int) { return *this; }

   private:
    size_t* count_;
  };

  iterator begin() { return iterator(count_); }

  size_t size() const { return count_; }

 private:
  size_t count_ = 0;
};

// Utility which provide a streaming interface to write data into an iterator.
// The main motivation of this stream is to encode integers into variable width
// format.
template <class Iterator>
class SinkStream {
  template <class T>
  static Iterator EncodeVarInt(T value, Iterator dst) {
    while (value >= 128) {
      *(dst++) = static_cast<uint8_t>(value) | 128;
      value >>= 7;
    }
    *(dst++) = static_cast<uint8_t>(value);
    return dst;
  }

 public:
  using iterator = Iterator;

  SinkStream() = default;
  explicit SinkStream(Iterator first) : data_(first) {}

  SinkStream& operator<<(uint8_t value) {
    *(data_++) = value;
    return *this;
  }
  // size_t is implicitly handled by uint32_t or uint64_t.
  SinkStream& operator<<(uint32_t value) {
    data_ = EncodeVarInt(value, data_);
    return *this;
  }
  SinkStream& operator<<(uint64_t value) {
    data_ = EncodeVarInt(value, data_);
    return *this;
  }
  SinkStream& operator<<(int32_t value) {
    if (value < 0)
      (*this) << uint32_t(~value * 2 + 1);
    else
      (*this) << uint32_t(value * 2);
    return *this;
  }
  SinkStream& operator<<(Region r) {
    std::copy(r.begin(), r.end(), data_);
    return *this;
  }

  template <class T>
  void operator()(T value) {
    (*this) << value;
  }

  Iterator& current() { return data_; }

 private:
  Iterator data_;
};

// Helper function that creates a sink stream writing to |it|.
template <class Iterator>
SinkStream<Iterator> MakeSinkStream(Iterator it) {
  return SinkStream<Iterator>(it);
}

// Utility which provide a streaming interface to read data from iterators.
// The main motivation of this stream is to decode integers from variable width
// format.
template <class Iterator>
class SourceStream {
 public:
  using iterator = Iterator;

  SourceStream() = default;
  SourceStream(Iterator first, Iterator last) : data_(first), last_(last) {}
  ~SourceStream() = default;

  template <class It, class T>
  static bool DecodeVarInt(It& lo, It hi, T* out) {  // NOLINT for It&.
    uint8_t sh = 0;
    T val = 0;
    while (lo < hi) {
      val |= T(*lo & 0x7F) << sh;
      if (*(lo++) < 0x80) {
        if (sh > sizeof(T) * 8)  // Overflow!
          return false;
        // Forgive overflow from the last byte.
        *out = val;
        return true;
      }
      sh += 7;
    }
    return false;
  }

  bool operator()(uint8_t* value) {
    if (data_ == last_)
      return false;
    *value = *(data_++);
    return true;
  }
  bool operator()(int8_t* value) {
    if (data_ == last_)
      return false;
    *value = *(data_++);
    return true;
  }
  // size_t is implicitly handled by uint32_t or uint64_t.
  bool operator()(uint32_t* value) { return DecodeVarInt(data_, last_, value); }
  bool operator()(uint64_t* value) { return DecodeVarInt(data_, last_, value); }
  bool operator()(int32_t* value) {
    uint32_t tmp = 0;
    if (!DecodeVarInt(data_, last_, &tmp))
      return false;
    if (tmp & 1)
      *value = ~int32_t(tmp >> 1);
    else
      *value = tmp >> 1;
    return true;
  }
  bool operator()(Region r) {
    size_t n = r.size();
    if (static_cast<size_t>(last_ - data_) < n)
      return false;
    std::copy_n(data_, n, r.begin());
    data_ += n;
    return true;
  }

  bool skip(size_t n) {
    if (static_cast<size_t>(last_ - data_) < n)
      return false;
    data_ += n;
    return true;
  }

  Iterator& current() { return data_; }
  Iterator& end() { return last_; }

 private:
  Iterator data_;
  Iterator last_;
};

// Helper function that creates a source stream reading from |first| and
// stopping at when reaching |last|.
template <class Iterator>
SourceStream<Iterator> MakeSourceStream(Iterator first, Iterator last) {
  return {first, last};
}

// Helper function that creates a source stream reading from |rng|.
template <class Rng>
SourceStream<typename ranges::range_traits<Rng>::iterator> MakeSourceStream(
    Rng&& rng) {
  return MakeSourceStream(std::begin(rng), std::end(rng));
}

// Holds buffers for multiple SinkStream and provide an interface to access
// these SinkStreams.
class SinkStreamSet {
 public:
  using inserter = std::back_insert_iterator<std::vector<uint8_t>>;
  using Stream = SinkStream<inserter>;

  SinkStreamSet();
  explicit SinkStreamSet(size_t count);
  ~SinkStreamSet();

  // Returns a Stream identified by |key|. The underlying buffer is owned by
  // SinkStreamSet, and created if nonexistent. The returned Stream object,
  // owned by the caller, is used to insert data the buffer.
  Stream Get(size_t key) {
    if (key >= data_.size())
      data_.resize(key + 1);
    return Stream(inserter(data_[key]));
  }

  // Serializes buffers of all its Streams into the single |sink|.
  // Extra data is also included to describe the size of the different streams,
  // which allows SourceStreamSet to retrieve them.
  template <class Iterator>
  void Serialize(SinkStream<Iterator>* sink) const {
    (*sink)(data_.size());
    for (const std::vector<uint8_t>& v : data_)
      (*sink)(v.size());
    for (const std::vector<uint8_t>& v : data_)
      sink->current() = ranges::copy(v, sink->current());
  }

 private:
  // Underlying stream storage. Using std::deque to prevent copying vectors on
  // resizing |data_|, which would invalidate Streams obtained from Get().
  std::deque<std::vector<uint8_t>> data_;
};

// Provide an interface to access multiple SourseStreams associated with a
// serialized Stream.
class SourceStreamSet {
 public:
  static constexpr uint32_t kMaxSourceStreamCount = 256;
  using Stream = SourceStream<Region::iterator>;

  SourceStreamSet();
  SourceStreamSet(const SourceStreamSet&);
  ~SourceStreamSet();

  // Initializes multiple Streams from their serialized version |src|, with an
  // associated buffer owned by the caller. Returns true on success, and false
  // otherwise.
  bool Init(Stream* src);

  size_t Count() const { return data_.size(); }

  // Returns a Stream identified by |key|.
  Stream Get(size_t key) const {
    return {data_[key].it, data_[key].it + data_[key].size};
  }

 private:
  struct Substream {
    Region::iterator it;
    size_t size;
  };
  std::vector<Substream> data_;
};

}  // namespace zucchini

#endif  // ZUCCHINI_STREAM_H_
