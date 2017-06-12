// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TRAFFIC_ANNOTATION_NETWORK_TRAFFIC_ANNOTATION_H_
#define CHROME_TRAFFIC_ANNOTATION_NETWORK_TRAFFIC_ANNOTATION_H_

#include "base/logging.h"

class ChromeNetworkTrafficAnnotation;

class NetworkTrafficAnnotation {
 public:
  int32_t GetUniqueIDHashCode() const { return unique_id_hash_code_; }

 private:
  NetworkTrafficAnnotation(int32_t unique_id_hash_code)
      : unique_id_hash_code_(unique_id_hash_code) {}
  const int32_t unique_id_hash_code_;

  friend class ChromeNetworkTrafficAnnotation;
};
// FROM THE BEGINNING OF FILE UNTIL HERE, GOES TO NET/TRAFFIC_ANNOTATION

// FROM HERE TO THE END OF FILE, GOES TO CHROME/TRAFFIC_ANNOTATION
class ChromeNetworkTrafficAnnotation {
 public:
  // TODO: Can we enforce this parameter to be constant string literal?
  ChromeNetworkTrafficAnnotation(const char* unique_id);

  operator NetworkTrafficAnnotation() const {
#if !defined(NDEBUG) || defined(DCHECK_ALWAYS_ON)
    DCHECK(policy_added_ && description_added_);
#endif
    return NetworkTrafficAnnotation(unique_id_hash_code_);
  }

  ChromeNetworkTrafficAnnotation& AddDescription(const char* descrciption);
  ChromeNetworkTrafficAnnotation& AddPolicy(const char* policy_name,
                                            const char* policy_value);

 private:
  const int32_t unique_id_hash_code_;

#if !defined(NDEBUG) || defined(DCHECK_ALWAYS_ON)
  bool description_added_;
  bool policy_added_;
#endif
};

#endif  // CHROME_TRAFFIC_ANNOTATION_NETWORK_TRAFFIC_ANNOTATION_H_
