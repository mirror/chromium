// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_OVERLAY_INFO_H_
#define MEDIA_BASE_OVERLAY_INFO_H_

#include "base/callback.h"
#include "base/optional.h"
#include "base/unguessable_token.h"
#include "media/base/media_export.h"
#include "media/base/surface_manager.h"

namespace media {

struct MEDIA_EXPORT OverlayInfo {
  // An unset routing token represents an invalid routing token (because empty
  // UnguessableTokens aren't serializable).
  using RoutingToken = base::Optional<base::UnguessableToken>;

  OverlayInfo();
  OverlayInfo(const OverlayInfo&);

  // Whether this contains either a valid surface id or routing token.
  bool HasValidOverlay() const;
  bool HasValidSurfaceId() const;
  bool HasValidRoutingToken() const;

  // Whether |other| refers to the same (surface_id, routing_token) pair as
  // |this|.
  bool RefersToSameOverlayAs(const OverlayInfo& other);

  // This is the SurfaceManager surface id, or SurfaceManager::kNoSurfaceID to
  // indicate that no surface from SurfaceManager should be used.
  int surface_id = SurfaceManager::kNoSurfaceID;

  // The routing token for AndroidOverlay, if any.
  RoutingToken routing_token;

  // Is the player in fullscreen?
  bool is_fullscreen = false;

  // Is the player's frame hidden / closed?
  bool is_frame_hidden = false;
};

using ProvideOverlayInfoCB = base::Callback<void(const OverlayInfo&)>;
using RequestOverlayInfoCB =
    base::Callback<void(bool, const ProvideOverlayInfoCB&)>;

}  // namespace media

#endif  // MEDIA_BASE_OVERLAY_INFO_H_
