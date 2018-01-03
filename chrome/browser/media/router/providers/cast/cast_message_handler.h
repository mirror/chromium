// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_CAST_CAST_MESSAGE_HANDLER_H_
#define CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_CAST_CAST_MESSAGE_HANDLER_H_

#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "base/values.h"
#include "components/cast_channel/cast_message_util.h"
#include "components/cast_channel/cast_socket.h"

namespace cast_channel {
class CastSocketService;
}

namespace media_router {

using GetAppAvailabilityCallback =
    base::OnceCallback<void(cast_channel::GetAppAvailabilityResult)>;

struct GetAppAvailabilityRequest {
  GetAppAvailabilityRequest(const std::string& app_id,
                            GetAppAvailabilityCallback callback);
  ~GetAppAvailabilityRequest();

  std::string app_id;
  GetAppAvailabilityCallback callback;
};

// TODO(imcheng): relocate this file to discovery/mdns.
class CastMessageHandler : public cast_channel::CastSocket::Observer {
 public:
  explicit CastMessageHandler(cast_channel::CastSocketService* socket_service);
  ~CastMessageHandler() override;

  // |callback| is always invoked asynchronously.
  void RequestAppAvailability(cast_channel::CastSocket* socket,
                              const std::string& app_id,
                              GetAppAvailabilityCallback callback);

 private:
  int NextRequestId() { return ++next_request_id_; }

  // cast_channel::CastSocket::Observer implementation.
  void OnError(const cast_channel::CastSocket& socket,
               cast_channel::ChannelError error_state) override;
  void OnMessage(const cast_channel::CastSocket& socket,
                 const cast_channel::CastMessage& message) override;

  void OnMessageSent(int result);

  base::flat_map<int, std::unique_ptr<GetAppAvailabilityRequest>>
      pending_app_availability_requests_;
  int next_request_id_ = 0;
  cast_channel::CastSocketService* const socket_service_;

  DISALLOW_COPY_AND_ASSIGN(CastMessageHandler);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_CAST_CAST_MESSAGE_HANDLER_H_
