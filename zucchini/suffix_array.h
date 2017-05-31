// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_SUFFIX_ARRAY_H_
#define ZUCCHINI_SUFFIX_ARRAY_H_

#include <algorithm>
#include <iterator>
#include <limits>
#include <memory>
#include <numeric>
#include <utility>
#include <vector>

#include "zucchini/ranges/algorithm.h"

namespace zucchini {

class Naive {
 public:
  template <class SA, class KeyType>
  static void Build(KeyType max_key, SA* sa) {
    using input_range = typename SA::input_range;
    using input_iterator =
        typename ranges::range_traits<input_range>::const_iterator;
    using size_type = typename SA::size_type;

    Impl<input_iterator, size_type, KeyType> impl(begin(sa->s_), max_key);
    size_type n = ranges::size(sa->sa_);
    impl.SufSort(begin(sa->sa_), n);
  }

 private:
  template <class InputIt, class SizeType, class KeyType = SizeType>
  struct Impl {
    using value_type = typename std::iterator_traits<InputIt>::value_type;
    using size_type = SizeType;
    using array = std::vector<size_type>;
    using iterator = typename array::iterator;
    using const_iterator = typename array::const_iterator;

    Impl(InputIt s, KeyType max_key) : s_(s) {}

    void SufSort(iterator sa, size_type n) {
      iota(sa, sa + n, 0);
      std::sort(sa, sa + n, [this, n](size_type i, size_type j) {
        return std::lexicographical_compare(s_ + i, s_ + n, s_ + j, s_ + n);
      });
    }

    InputIt s_;
  };
};

class Sais {
 public:
  template <class SA, class SizeType>
  static void Build(SizeType max_key, SA* sa) {
    using ranges::begin;
    using ranges::end;
    using input_range = typename SA::input_range;
    using input_iterator =
        typename ranges::range_traits<input_range>::const_iterator;
    using size_type = typename SA::size_type;
    Impl<input_iterator, size_type> impl(begin(sa->s_), size_type(max_key));
    size_type n = size_type(ranges::size(sa->sa_));
    impl.SufSort(begin(sa->sa_), n);
  }

 private:
  template <class InputIt,
            class SizeType,
            class KeyType = typename std::iterator_traits<InputIt>::value_type>
  struct Impl {
    enum SLType : bool {
      SType,
      LType,
    };

    using value_type = typename std::iterator_traits<InputIt>::value_type;
    using size_type = SizeType;
    using key_type = KeyType;
    using array = std::vector<size_type>;
    using iterator = typename array::iterator;
    using const_iterator = typename array::const_iterator;

    Impl(InputIt s, SizeType max_key) : s_(s), buckets_(size_type(max_key)) {}

    void SufSort(iterator sa, size_type n) {
      t_.resize(n);
      lms_.resize(MakeSLPartition(n));
      MakeLms(n);
      BucketCount(n);

      if (lms_.size() > 1) {
        InducedSort(sa, n);
        std::vector<size_type> s1(lms_.size());
        auto last_label = LabelLms(sa, n, begin(lms_), begin(s1));
        if (last_label < lms_.size()) {
          for (size_type i = 0; i < lms_.size(); ++i)
            sa[lms_[i]] = s1[i];
          bool ptype = SType;
          size_type j = 0;
          for (size_type i = 0; i < t_.size(); ++i) {
            if (t_[i] == SType && ptype == LType) {
              s1[j] = sa[i];
              lms_[j++] = i;
            }
            ptype = t_[i];
          }

          Impl<iterator, size_type> impl(begin(s1), last_label);
          impl.SufSort(sa, size_type(lms_.size()));
          for (size_type i = 0; i < lms_.size(); ++i)
            sa[i] = lms_[sa[i]];
          for (size_type i = 0; i < lms_.size(); ++i)
            lms_[i] = sa[i];
        }
      }
      InducedSort(sa, n);
    }

    void InducedSort(iterator sa, size_type n) {
      using ranges::begin;
      using ranges::end;
      size_type max_key = size_type(buckets_.size());
      fill(sa, sa + n, 0);
      std::vector<size_type> idx(max_key);

      idx[0] = buckets_[0];
      for (size_type i = 1; i < max_key; ++i)
        idx[i] = idx[i - 1] + buckets_[i];
      for (auto i = lms_.crbegin(); i != lms_.crend(); ++i) {
        key_type key = s_[*i];
        sa[--idx[key]] = *i;
      }

      idx[0] = 0;
      for (size_type i = 1; i < max_key; ++i)
        idx[i] = idx[i - 1] + buckets_[i - 1];
      if (t_[n - 1] == LType) {
        key_type key = s_[n - 1];
        sa[idx[key]++] = n - 1;
      }
      for (auto i = sa; i != sa + n; ++i) {
        size_type j = *i;
        if (j > 0 && t_[--j] == LType) {
          key_type key = s_[j];
          sa[idx[key]++] = j;
        }
      }

      idx[0] = buckets_[0];
      for (size_type i = 1; i < max_key; ++i)
        idx[i] = idx[i - 1] + buckets_[i];
      for (auto i = ranges::make_reverse_iterator(sa + n);
           i != ranges::make_reverse_iterator(sa); ++i) {
        size_type j = *i;
        if (j > 0 && t_[--j] == SType) {
          key_type key = s_[j];
          sa[--idx[key]] = j;
        }
      }
      if (t_[n - 1] == SType) {
        key_type key = s_[n - 1];
        sa[--idx[key]] = n - 1;
      }
    }
    size_type LabelLms(const_iterator sa,
                       size_type n,
                       iterator lms,
                       iterator labels) {
      size_type label = 0;
      size_type plms = 0;
      for (auto i = sa; i != sa + n; ++i) {
        if (*i > 0 && t_[*i] == SType && t_[*i - 1] == LType) {
          size_type clms = *i;
          if (plms != 0) {
            bool clms_type = SType, plms_type = SType;
            for (size_type k = 0;; ++k) {
              bool clms_end = false, plms_end = false;
              if (clms + k >= n ||
                  (clms_type == LType && t_[clms + k] == SType)) {
                clms_end = true;
              }
              if (plms + k >= n ||
                  (plms_type == LType && t_[plms + k] == SType)) {
                plms_end = true;
              }

              if (clms_end && plms_end) {
                break;
              } else if (clms_end != plms_end || s_[clms + k] != s_[plms + k]) {
                ++label;
                break;
              }

              clms_type = t_[clms + k];
              plms_type = t_[plms + k];
            }
          }
          *(lms++) = *i;
          *(labels++) = label;
          plms = clms;
        }
      }
      return ++label;
    }
    size_type MakeSLPartition(size_type n) {
      using ranges::begin;
      using ranges::end;
      SLType ptype = LType;
      size_type pkey = size_type(buckets_.size());
      size_type count = 0;
      auto t = t_.rbegin();
      for (auto i = ranges::make_reverse_iterator(s_ + n);
           i != ranges::make_reverse_iterator(s_); ++i, ++t) {
        key_type ckey = *i;
        if (ckey > pkey || pkey == buckets_.size()) {
          *t = static_cast<bool>(LType);
          if (ptype == SType)
            ++count;
          ptype = LType;
        } else if (ckey < pkey) {
          *t = static_cast<bool>(SType);
          ptype = SType;
        } else {
          *t = static_cast<bool>(ptype == LType);
        }
        pkey = size_type(ckey);
      }
      return count;
    }
    void MakeLms(size_type n) {
      bool ptype = SType;
      size_type j = 0;
      for (size_type i = 0; i < n; ++i) {
        if (t_[i] == SType && ptype == LType)
          lms_[j++] = i;
        ptype = t_[i];
      }
    }
    void BucketCount(size_type n) {
      for (auto i = s_; i != s_ + n; ++i)
        ++buckets_[*i];
    }

    InputIt s_;
    array buckets_;
    array lms_;
    std::vector<bool> t_;
  };
};

template <class InputRng>
class SuffixArray {
  template <class>
  friend class SuffixArray;
  friend class Sais;

 public:
  using size_type = typename ranges::range_traits<InputRng>::size_type;
  using difference_type =
      typename ranges::range_traits<InputRng>::difference_type;
  using input_range = InputRng;

  size_type Size() const { return sa_.size(); }

  explicit SuffixArray(InputRng&& s) : s_(std::forward<InputRng>(s)) {
    using ranges::begin;
    using ranges::end;
    size_type n = ranges::size(s);
    sa_.resize(n);
  }

  template <class InputRng2>
  std::pair<size_type, size_type> Search(InputRng2&& s) const {
    using ranges::begin;
    using ranges::end;
    return Search(begin(s), end(s));
  }

  template <class InputIt, class Sentinel>
  std::pair<size_type, size_type> Search(InputIt x, Sentinel sent) const {
    using ranges::begin;
    using ranges::end;
    auto s = begin(s_);
    size_t n = sa_.size();
    auto it = std::lower_bound(
        begin(sa_), end(sa_), x, [s, n, sent](size_type a, InputIt b) {
          return std::lexicographical_compare(s + a, s + n, b, sent);
        });

    size_type n1 = 0;
    size_type n2 = 0;
    if (it != end(sa_)) {
      n1 = ranges::mismatch(s + *it, s + n, x, sent).second - x;
    }
    if (it != begin(sa_)) {
      n2 = ranges::mismatch(s + *(it - 1), s + n, x, sent).second - x;
    }
    if (n2 > n1 || it == end(sa_)) {
      --it;
      n1 = n2;
    }
    return std::make_pair(*it, n1);
  }

  const std::vector<size_type>& Get() const { return sa_; }
  const std::vector<size_type>& GetInverse() const { return isa_; }
  const std::vector<size_type>& GetLcp() const { return lcp_; }

  size_type at(size_type i) const { return sa_[i]; }
  size_type of(size_type i) const { return isa_[i]; }
  size_type lcp(size_type i) const { return lcp_[i]; }

  void MakeInverse() {
    isa_.resize(sa_.size());
    for (size_type i = 0; i < Size(); ++i)
      isa_[sa_[i]] = i;
  }

  void MakeLcp() {
    using ranges::begin;
    if (!isa_.size())
      MakeInverse();
    lcp_.resize(Size());
    size_type k = 0;
    auto s = begin(s_);
    for (size_type i = 0; i < Size(); ++i, k ? --k : 0) {
      if (isa_[i] == sa_.size() - 1) {
        k = 0;
        continue;
      }
      size_type j = sa_[isa_[i] + 1];
      while (i + k < Size() && j + k < Size() && s[i + k] == s[j + k])
        k++;
      lcp_[isa_[i]] = k;
    }
  }

 private:
  InputRng&& s_;
  std::vector<size_type> sa_;
  std::vector<size_type> isa_;
  std::vector<size_type> lcp_;
};

template <class Algorithm = Sais, class InputRng, class KeyType>
std::unique_ptr<SuffixArray<InputRng>> MakeSuffixArray(InputRng&& s,
                                                       KeyType max_key) {
  std::unique_ptr<SuffixArray<InputRng>> sa(
      new SuffixArray<InputRng>(std::forward<InputRng>(s)));
  Algorithm::Build(max_key, sa.get());
  return sa;
}

template <class Algorithm = Sais, class InputRng>
std::unique_ptr<SuffixArray<InputRng>> MakeSuffixArray(InputRng&& s) {
  using value_type = typename ranges::range_traits<InputRng>::value_type;
  std::unique_ptr<SuffixArray<InputRng>> sa(
      new SuffixArray<InputRng>(std::forward<InputRng>(s)));
  Algorithm::Build(std::numeric_limits<value_type>::max(), sa.get());
  return sa;
}

}  // namespace zucchini

#endif  // ZUCCHINI_SUFFIX_ARRAY_H_
