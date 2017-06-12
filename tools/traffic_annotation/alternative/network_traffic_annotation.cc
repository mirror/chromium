// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/traffic_annotation/alternative/network_traffic_annotation.h"

namespace {

// // Recursively compute hash code of the given string as a constant
// expression. template <int N> constexpr uint32_t recursive_hash(const char*
// str) {
//   return static_cast<uint32_t>((recursive_hash<N - 1>(str) * 31 + str[N - 1])
//   %
//                                138003713);
// }

// // Recursion stopper for the above function. Note that string of size 0 will
// // result in compile error.
// template <>
// constexpr uint32_t recursive_hash<1>(const char* str) {
//   return static_cast<uint32_t>(*str);
// }

// // Entry point to function that computes hash as constant expression.
// #define COMPUTE_STRING_HASH(S) \
//   static_cast<int32_t>(recursive_hash<sizeof(S) - 1>(S))

#define TEMP_COMPUTE_HASH(S) static_cast<int32_t>(S[0])

}  // namespace

ChromeNetworkTrafficAnnotation::ChromeNetworkTrafficAnnotation(
    const char* unique_id)
    : unique_id_hash_code_(TEMP_COMPUTE_HASH(unique_id))
#if !defined(NDEBUG) || defined(DCHECK_ALWAYS_ON)
      ,
      description_added_(false),
      policy_added_(false)
#endif
{
}

ChromeNetworkTrafficAnnotation& ChromeNetworkTrafficAnnotation::AddDescription(
    const char* descrciption) {
#if !defined(NDEBUG) || defined(DCHECK_ALWAYS_ON)
  description_added_ = true;
#endif
  return *this;
}
ChromeNetworkTrafficAnnotation& ChromeNetworkTrafficAnnotation::AddPolicy(
    const char* policy_name,
    const char* policy_value) {
#if !defined(NDEBUG) || defined(DCHECK_ALWAYS_ON)
  policy_added_ = true;
#endif
  return *this;
}

//
// TEST AREA:
//
void NetworkFunction(const NetworkTrafficAnnotation& traffic_annotation) {
  printf("Running %i\n", traffic_annotation.GetUniqueIDHashCode());
}

int main(int argc, char* argv[]) {
  // OK
  ChromeNetworkTrafficAnnotation good_annotation =
      ChromeNetworkTrafficAnnotation("Hello World")
          .AddDescription(
              "Description Description Description Description Description"
              "Description Description Description Description Description"
              "Description Description Description Description Description"
              "Description Description Description Description Description")
          .AddPolicy("Policy", "Value");

  NetworkFunction(good_annotation);

  // NOT OK in DEBUG
  ChromeNetworkTrafficAnnotation bad_annotation =
      ChromeNetworkTrafficAnnotation("Hello World");

  NetworkFunction(bad_annotation);
}