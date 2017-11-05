// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/cast/parsed_media_source.h"

#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/common/media_router/media_source_helper.h"
#include "url/gurl.h"
#include "url/url_util.h"

namespace media_router {

namespace {

constexpr char kMirroringAppId[] = "0F5096E8";
constexpr char kAudioMirroringAppId[] = "85CDB22F";

// Parameter keys used by new Cast URLs.
constexpr char kCapabilitiesKey[] = "capabilities";
constexpr char kBroadcastNamespaceKey[] = "broadcastNamespace";
constexpr char kBroadcastMessageKey[] = "broadcastMessage";

// Parameter keys used by legacy Cast URLs.
constexpr char kLegacyCastAppIdKey[] = "__castAppId__";
constexpr char kLegacyBroadcastNamespaceKey[] = "__castBroadcastNamespace__";
constexpr char kLegacyBroadcastMessageKey[] = "__castBroadcastMessage__";

// TODO(imcheng): Move to common utils?
std::string DecodeURLComponent(const std::string& encoded) {
  url::RawCanonOutputT<base::char16> unescaped;
  std::string output;
  url::DecodeURLEscapeSequences(encoded.data(), encoded.size(), &unescaped);
  if (base::UTF16ToUTF8(unescaped.data(), unescaped.length(), &output))
    return output;

  return std::string();
}

cast_channel::CastDeviceCapability CastDeviceCapabilityFromString(
    const base::StringPiece& s) {
  if (s == "video_out")
    return cast_channel::CastDeviceCapability::VIDEO_OUT;
  if (s == "video_in")
    return cast_channel::CastDeviceCapability::VIDEO_IN;
  if (s == "audio_out")
    return cast_channel::CastDeviceCapability::AUDIO_OUT;
  if (s == "audio_in")
    return cast_channel::CastDeviceCapability::AUDIO_IN;
  if (s == "multizone_group")
    return cast_channel::CastDeviceCapability::MULTIZONE_GROUP;

  return cast_channel::CastDeviceCapability::NONE;
}

std::unique_ptr<ParsedMediaSource> ParsedMediaSourceForTabMirroring() {
  return std::make_unique<ParsedMediaSource>(std::vector<CastAppInfo>(
      {CastAppInfo(kMirroringAppId), CastAppInfo(kAudioMirroringAppId)}));
}

std::unique_ptr<ParsedMediaSource> ParsedMediaSourceForDesktopMirroring() {
// Desktop audio mirroring is only supported on some platforms.
#if defined(OS_WIN) || defined(OS_CHROMEOS)
  return std::make_unique<ParsedMediaSource>(std::vector<CastAppInfo>(
      {CastAppInfo(kMirroringAppId), CastAppInfo(kAudioMirroringAppId)}));
#else
  return std::make_unique<ParsedMediaSource>(
      std::vector<CastAppInfo>({CastAppInfo(kMirroringAppId)}));
#endif
}

std::unique_ptr<ParsedMediaSource> ParseCastUrl(const GURL& url) {
  std::string app_id = url.host();
  // App ID must be non-empty.
  if (app_id.empty())
    return nullptr;

  base::StringPairs parameters;
  base::SplitStringIntoKeyValuePairs(url.query(), '=', '&', &parameters);
  std::string broadcast_namespace, broadcast_message, capabilities;
  for (const auto& key_value : parameters) {
    const auto& key = key_value.first;
    const auto& value = key_value.second;
    if (key == kBroadcastNamespaceKey) {
      broadcast_namespace = value;
    } else if (key == kBroadcastMessageKey) {
      // The broadcast message is URL-encoded.
      broadcast_message = DecodeURLComponent(value);
    } else if (key == kCapabilitiesKey) {
      capabilities = value;
    }
  }

  CastAppInfo app_info(app_id);
  if (!capabilities.empty()) {
    for (const auto& capability :
         base::SplitStringPiece(capabilities, ",", base::KEEP_WHITESPACE,
                                base::SPLIT_WANT_NONEMPTY)) {
      app_info.capabilities |= CastDeviceCapabilityFromString(capability);
    }
  }

  auto parsed = std::make_unique<ParsedMediaSource>(app_info);
  if (!broadcast_namespace.empty() && !broadcast_message.empty())
    parsed->broadcast_request =
        BroadcastRequest(broadcast_namespace, broadcast_message);

  return parsed;
}

std::unique_ptr<ParsedMediaSource> ParseLegacyCastUrl(const GURL& url) {
  base::StringPairs parameters;
  base::SplitStringIntoKeyValuePairs(url.ref(), '=', '/', &parameters);
  // Legacy URLs can specify multiple apps.
  std::vector<std::string> app_id_params;
  std::string broadcast_namespace, broadcast_message;
  for (const auto& key_value : parameters) {
    const auto& key = key_value.first;
    if (key == kLegacyCastAppIdKey) {
      app_id_params.push_back(key_value.second);
    } else if (key == kLegacyBroadcastNamespaceKey) {
      broadcast_namespace = key_value.second;
    } else if (key == kLegacyBroadcastMessageKey) {
      // The broadcast message is URL-encoded.
      broadcast_message = DecodeURLComponent(key_value.second);
    }
  }

  std::vector<CastAppInfo> app_infos;
  for (const auto& app_id_param : app_id_params) {
    std::string app_id;
    std::string capabilities;
    auto cap_start_index = app_id_param.find('(');
    app_id = app_id_param.substr(0, cap_start_index);
    if (cap_start_index != std::string::npos) {
      auto cap_end_index = app_id_param.find(')', cap_start_index);
      if (cap_end_index != std::string::npos &&
          cap_end_index > cap_start_index) {
        capabilities = app_id_param.substr(cap_start_index, cap_end_index);
      }
    }
    if (app_id.empty())
      continue;

    CastAppInfo app_info(app_id);
    for (const auto& capability :
         base::SplitStringPiece(capabilities, ",", base::KEEP_WHITESPACE,
                                base::SPLIT_WANT_NONEMPTY)) {
      app_info.capabilities |= CastDeviceCapabilityFromString(capability);
    }

    app_infos.push_back(app_info);
  }

  if (app_infos.empty())
    return nullptr;

  auto parsed = std::make_unique<ParsedMediaSource>(std::move(app_infos));
  return parsed;
}

}  // namespace

CastAppInfo::CastAppInfo(const CastAppId& app_id) : app_id(app_id) {}
CastAppInfo::~CastAppInfo() = default;

CastAppInfo::CastAppInfo(const CastAppInfo& other) = default;

BroadcastRequest::BroadcastRequest(const std::string& broadcast_namespace,
                                   const std::string& message)
    : broadcast_namespace(broadcast_namespace), message(message) {}
BroadcastRequest::~BroadcastRequest() = default;

// static
std::unique_ptr<ParsedMediaSource> ParsedMediaSource::From(
    const MediaSource& source) {
  if (IsTabMirroringMediaSource(source))
    return ParsedMediaSourceForTabMirroring();

  if (IsDesktopMirroringMediaSource(source))
    return ParsedMediaSourceForDesktopMirroring();

  const GURL& url = source.url();
  if (!url.is_valid())
    return nullptr;

  if (url.SchemeIs(kCastPresentationUrlScheme)) {
    return ParseCastUrl(url);
  } else if (IsLegacyCastPresentationUrl(url)) {
    return ParseLegacyCastUrl(url);
  } else if (url.SchemeIsHTTPOrHTTPS()) {
    // Arbitrary https URLs are supported via 1-UA mode which uses tab
    // mirroring.
    return ParsedMediaSourceForTabMirroring();
  }

  return nullptr;
}

ParsedMediaSource::ParsedMediaSource(const CastAppInfo& app_info)
    : app_infos({app_info}) {}
ParsedMediaSource::ParsedMediaSource(const std::vector<CastAppInfo>& app_infos)
    : app_infos(app_infos) {}
ParsedMediaSource::ParsedMediaSource(const ParsedMediaSource& other) = default;
ParsedMediaSource::~ParsedMediaSource() = default;

bool ParsedMediaSource::ContainsApp(CastAppId app_id) const {
  for (const auto& info : app_infos) {
    if (info.app_id == app_id)
      return true;
  }
  return false;
}

}  // namespace media_router
