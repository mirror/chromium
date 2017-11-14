// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_CAST_PARSED_MEDIA_SOURCE_H_
#define CHROME_BROWSER_MEDIA_ROUTER_CAST_PARSED_MEDIA_SOURCE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/optional.h"
#include "components/cast_channel/cast_socket.h"

namespace media_router {

class MediaSource;

using CastAppId = std::string;

struct CastAppInfo {
  explicit CastAppInfo(const CastAppId& app_id);
  ~CastAppInfo();

  CastAppInfo(const CastAppInfo& other);

  std::string app_id;

  // A bitset of cast_channel::CastDeviceCapabilty values. It is reasonable to
  // assume that its std::underlying_type, which is int, is unlikely to change.
  int capabilities = cast_channel::CastDeviceCapability::NONE;
};

struct BroadcastRequest {
  BroadcastRequest(const std::string& broadcast_namespace,
                   const std::string& message);
  ~BroadcastRequest();

  std::string broadcast_namespace;
  std::string message;
};

struct ParsedMediaSource {
  static std::unique_ptr<ParsedMediaSource> From(const MediaSource& source);

  explicit ParsedMediaSource(const CastAppInfo& app_info);
  explicit ParsedMediaSource(const std::vector<CastAppInfo>& app_infos);
  ParsedMediaSource(const ParsedMediaSource& other);
  ~ParsedMediaSource();

  bool ContainsApp(CastAppId app_id) const;

  // TODO(imcheng): Backfill other parameters.
  std::vector<CastAppInfo> app_infos;
  base::Optional<BroadcastRequest> broadcast_request;
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_CAST_PARSED_MEDIA_SOURCE_H_
