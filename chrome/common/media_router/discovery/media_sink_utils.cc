// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/media_router/discovery/media_sink_utils.h"

#include <cmath>

#include "base/base64url.h"
#include "base/logging.h"
#include "base/rand_util.h"
#include "base/sha1.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"

namespace media_router {

namespace {

constexpr char kBase36[] = "0123456789abcdefghijklmnopqrstuvwxyz";

std::string ToBase36(long val) {
  std::string result;
  do {
    result = kBase36[val % 36] + result;
    val = val / 36;
  } while (val > 0);
  return result;
}

// Returns a random string.
std::string GetRandomString() {
  long x = 2147483648;
  long tick = base::Time::Now().ToJsTime();
  long part1 = std::floor(base::RandDouble() * x);
  long part2 =
      std::abs(static_cast<long>(std::floor(base::RandDouble() * x)) ^ tick);
  return ToBase36(part1) + ToBase36(part2);
}

// Returns processed uuid without "uuid:" and "-", e.g. input
// "uuid:6d238518-a574-eab1-017e-d0975c039081" and output
// "6d238518a574eab1017ed0975c039081"
std::string ProcessUUID(const std::string& unique_id) {
  if (unique_id.empty())
    return std::string();

  std::string result = unique_id;
  if (unique_id.find("uuid:") == 0)
    result = unique_id.substr(5);

  base::RemoveChars(result, "-", &result);
  return base::ToLowerASCII(result);
}

}  // namespace

MediaSinkUtils::MediaSinkUtils() : receiver_id_token_(GetRandomString()) {}
MediaSinkUtils::MediaSinkUtils(const std::string& receiver_id_token)
    : receiver_id_token_(receiver_id_token) {}
MediaSinkUtils::~MediaSinkUtils() = default;

std::string MediaSinkUtils::GenerateId(const std::string& device_uuid) {
  std::string processed_uuid = ProcessUUID(device_uuid);
  std::string sha1_hash =
      base::SHA1HashString(processed_uuid + receiver_id_token_);
  base::Base64UrlEncode(sha1_hash, base::Base64UrlEncodePolicy::OMIT_PADDING,
                        &sha1_hash);
  return "browser_" + sha1_hash;
}

}  // namespace media_router
