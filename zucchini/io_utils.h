// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_IO_UTILS_H_
#define ZUCCHINI_IO_UTILS_H_

#include <cctype>
#include <iostream>
#include <sstream>
#include <string>

namespace zucchini {

// An std::ostream wrapper that limits number of lines to output, typically to
// print debug message while avoiding excessive output. Sample usage:
//
// static LimitedOutputStream los(std::cerr, 10);
// if (!los.full()) {
//   ...  // Prepare message. Must be non-intrusive since block may be skipped.
//   los << message;
//   los << std::endl;  // Important!
// }
class LimitedOutputStream : public std::ostream {
 private:
  class StreamBuf : public std::stringbuf {
   public:
    StreamBuf(std::ostream& os, int limit);
    ~StreamBuf();

    int sync() override;
    bool full() const { return counter_ >= limit_; }

   private:
    std::ostream& os_;
    const int limit_;
    int counter_ = 0;
  };

 public:
  LimitedOutputStream(std::ostream& os, int limit);
  bool full() const { return buf_.full(); }

 private:
  StreamBuf buf_;
};

// A class to render hexdecimal numbers for std::ostream with 0-padding. This is
// more concise and flexible than the stateful STL manipulator alternative; so:
//   std::ios old_fmt(nullptr);
//   old_fmt.copyfmt(std::cout);
//   std::cout << std::uppercase << std::hex;
//   std::cout << std::setfill('0') << std::setw(8) << int_data << std::endl;
//   std::cout.copyfmt(old_fmt);
// can be expressed as:
//   std::cout << AxHex<8>(int_data) << std::endl;
template <int N, typename T = uint32_t>
struct AsHex {
  explicit AsHex(T v_in) : v(v_in) {}
  T v;
};

template <int N, typename T>
std::ostream& operator<<(std::ostream& os, const AsHex<N, T>& as_hex) {
  char buf[N + 1];
  buf[N] = '\0';
  T v = as_hex.v;
  for (int i = N - 1; i >= 0; --i, v >>= 4)
    buf[i] = "0123456789ABCDEF"[static_cast<int>(v & 0x0F)];
  if (v)
    os << "...";  // To indicate data truncation, or negative values.
  os << buf;
  return os;
}

// An output manipulator to simplify printing list separators. Sample usage:
//   PrefixSep sep(",");
//   for (int i : {3, 1, 4, 1, 5, 9})
//      std::cout << sep << i;
//   std::cout << std::endl;  // Outputs "3,1,4,1,5,9\n".
struct PrefixSep {
  explicit PrefixSep(const std::string& sep_str_in) : sep_str(sep_str_in) {}

  std::string sep_str;
  bool first = true;
};

inline std::ostream& operator<<(std::ostream& ostr, PrefixSep& obj) {
  if (obj.first)
    obj.first = false;
  else
    ostr << obj.sep_str;
  return ostr;
}

// An input manipulator that dictates the expected next charactor in
// |std::istream|, and invalidates the stream if expectation is not met.
struct EatChar {
  explicit EatChar(char ch_in) : ch(ch_in) {}

  char ch;
};

inline std::istream& operator>>(std::istream& istr, const EatChar& obj) {
  if (!istr.fail() && istr.get() != obj.ch)
    istr.setstate(std::ios_base::failbit);
  return istr;
}

// An input manipulator that reads an unsigned integer from |std::istream|,
// and invalidates the stream on failure. Intolerant of leading white spaces,
template <typename T>
struct StrictUInt {
  explicit StrictUInt(T& var_in) : var(var_in) {}

  T& var;
};

template <typename T>
std::istream& operator>>(std::istream& istr, StrictUInt<T> obj) {
  if (!istr.fail() && !::isdigit(istr.peek())) {
    istr.setstate(std::ios_base::failbit);
    return istr;
  }
  return istr >> obj.var;
}

}  // namespace zucchini

#endif  // ZUCCHINI_IO_UTILS_H_
