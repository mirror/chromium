// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CAST_CHANNEL_CAST_MESSAGE_UTIL_H_
#define COMPONENTS_CAST_CHANNEL_CAST_MESSAGE_UTIL_H_

#include <string>

#include "base/values.h"

namespace cast_channel {

class AuthContext;
class CastMessage;
class DeviceAuthMessage;

// Sender and receiver IDs to use for platform messages.
constexpr char kPlatformSenderId[] = "sender-0";
constexpr char kPlatformReceiverId[] = "receiver-0";

// Message namespaces.
constexpr char kAuthNamespace[] = "urn:x-cast:com.google.cast.tp.deviceauth";
constexpr char kReceiverNamespace[] = "urn:x-cast:com.google.cast.receiver";

// Message types.
constexpr char kGetAppAvailabilityRequestType[] = "GET_APP_AVAILABILITY";

// Checks if the contents of |message_proto| are valid.
bool IsCastMessageValid(const CastMessage& message_proto);

// Parses and returns the UTF-8 payload from |message|. Returns an empty
// nullptr if the UTF-8 payload doesn't exist, or if it is not a dictionary.
std::unique_ptr<base::Value> GetDictionaryFromCastMessage(
    const CastMessage& message);

// Returns a human readable string for |message_proto|.
std::string CastMessageToString(const CastMessage& message_proto);

// Returns a human readable string for |message|.
std::string AuthMessageToString(const DeviceAuthMessage& message);

// Fills |message_proto| appropriately for an auth challenge request message.
// Uses the nonce challenge in |auth_context|.
void CreateAuthChallengeMessage(CastMessage* message_proto,
                                const AuthContext& auth_context);

// Returns whether the given message is an auth handshake message.
bool IsAuthMessage(const CastMessage& message);

// Possible results of a GET_APP_AVAILABILITY request.
enum class GetAppAvailabilityResult {
  kAvailable,
  kUnavailable,
  kUnknown,
};

// Extracts request ID from |payload| corresponding to a Cast message response.
// If request ID is available, assigns it to |request_id|. Return |true| if
// request ID is found.
bool GetRequestIdFromResponse(const base::Value& payload, int* request_id);

// Creates a CastMessage corresponding to a GET_APP_AVAILABILITY request with
// the given arguments.
CastMessage CreateGetAppAvailabilityRequest(int request_id,
                                            const std::string& app_id);

// Returns the GetAppAvailabilityResult corresponding to |app_id| in |payload|.
// Rreturns kUnknown if result is not found.
GetAppAvailabilityResult GetAppAvailabilityResultFromResponse(
    const base::Value& payload,
    const std::string& app_id);

}  // namespace cast_channel

#endif  // COMPONENTS_CAST_CHANNEL_CAST_MESSAGE_UTIL_H_
