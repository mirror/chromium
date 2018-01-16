// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_CAST_PARSED_MEDIA_SOURCE_H_
#define CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_CAST_PARSED_MEDIA_SOURCE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/optional.h"
#include "chrome/common/media_router/media_source.h"
#include "components/cast_channel/cast_socket.h"

namespace media_router {

struct CastAppInfo {
  explicit CastAppInfo(const std::string& app_id);
  ~CastAppInfo();

  CastAppInfo(const CastAppInfo& other);

  std::string app_id;

  // A bitset of cast_channel::CastDeviceCapabilty values.
  int capabilities = cast_channel::CastDeviceCapability::NONE;
};

struct BroadcastRequest {
  BroadcastRequest(const std::string& broadcast_namespace,
                   const std::string& message);
  ~BroadcastRequest();

  std::string broadcast_namespace;
  std::string message;
};

// Represents a MediaSource parsed into structured, Cast specific data. The
// following MediaSource can be parsed into ParsedMediaSource:
// - New / legacy Cast URLs
// - Desktop / tab mirroring URNs
struct ParsedMediaSource {
  // Returns the parsed form of |source|, or nullptr if it cannot be parsed.
  static std::unique_ptr<ParsedMediaSource> From(const MediaSource::Id& source);

  explicit ParsedMediaSource(const MediaSource::Id& source_id,
                             const CastAppInfo& app_info);
  explicit ParsedMediaSource(const MediaSource::Id& source_id,
                             const std::vector<CastAppInfo>& app_infos);
  ParsedMediaSource(const ParsedMediaSource& other);
  ~ParsedMediaSource();

  // Returns |true| if |app_infos| contain |app_id|.
  bool ContainsApp(const std::string& app_id) const;

  // TODO(imcheng): Backfill other parameters.
  MediaSource::Id source_id;
  std::vector<CastAppInfo> app_infos;
  base::Optional<BroadcastRequest> broadcast_request;
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_CAST_PARSED_MEDIA_SOURCE_H_
