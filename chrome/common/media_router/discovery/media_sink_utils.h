// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_MEDIA_ROUTER_DISCOVERY_MEDIA_SINK_UTILS_H_
#define CHROME_COMMON_MEDIA_ROUTER_DISCOVERY_MEDIA_SINK_UTILS_H_

#include <string>

namespace media_router {

// Utility class for media sinks.
class MediaSinkUtils {
 public:
  // Sets |receiver_id_token_| to a random string.
  MediaSinkUtils();
  explicit MediaSinkUtils(const std::string& receiver_id_token);
  ~MediaSinkUtils();

  std::string receiver_id_token() const { return receiver_id_token_; }

  // Returns a generated sink ID. Always returns the same sink ID for the same
  // |device_uuid|.
  // |device_uuid|: unique id of discovered device.
  std::string GenerateId(const std::string& device_uuid);

 private:
  // A random string used to generate per-profile media sink ids.
  const std::string receiver_id_token_;
};

}  // namespace media_router

#endif  // CHROME_COMMON_MEDIA_ROUTER_DISCOVERY_MEDIA_SINK_UTILS_H_
