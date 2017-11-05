// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_CAST_CAST_REQUESTS_H_
#define CHROME_BROWSER_MEDIA_ROUTER_CAST_CAST_REQUESTS_H_

#include "base/macros.h"
#include "base/values.h"
#include "chrome/browser/media/router/cast/parsed_media_source.h"
#include "components/cast_channel/cast_socket.h"

namespace cast_channel {
class CastSocketService;
}

namespace media_router {

enum class GetAppAvailabilityResult {
  kAvailable,
  kUnavailable,
  kUnknown,
  // Timeout, network error etc.
  kError
};

class CastMessageSender : public cast_channel::CastSocket::Observer {
 public:
  using GetAppAvailabilityCallback =
      base::OnceCallback<void(GetAppAvailabilityResult)>;

  explicit CastMessageSender(cast_channel::CastSocketService* socket_service);
  ~CastMessageSender() override;

  // |callback| is always invoked asynchronously.
  void RequestAppAvailability(const cast_channel::CastSocket& socket,
                              const CastAppId& app_id,
                              GetAppAvailabilityCallback callback);

 private:
  int NextRequestId() { return ++next_request_id_; }

  // cast_channel::CastSocket::Observer implementation.
  void OnError(const cast_channel::CastSocket& socket,
               cast_channel::ChannelError error_state) override;
  void OnMessage(const cast_channel::CastSocket& socket,
                 const cast_channel::CastMessage& message) override;

  int next_request_id_ = 0;
  cast_channel::CastSocketService* const socket_service_;

  DISALLOW_COPY_AND_ASSIGN(CastMessageSender);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_CAST_CAST_REQUESTS_H_
