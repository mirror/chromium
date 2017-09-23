// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cast_channel/cast_socket.h"

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/guid.h"
#include "base/memory/ptr_util.h"
#include "base/synchronization/waitable_event.h"
#include "base/test/scoped_feature_list.h"
#include "base/threading/thread.h"
#include "base/time/default_clock.h"
#include "components/cast_channel/cast_channel_enum.h"
#include "components/cast_channel/logger.h"
#include "net/log/net_log.h"

namespace {
const int kCastPort = 8009;

#define CAST_CHANNEL_TYPE_TO_STRING(enum) \
  case enum:                              \
    return #enum
}  // namespace

namespace cast_channel {

std::string ChallengeReplyErrorToString(ChallengeReplyError challenge_error) {
  switch (challenge_error) {
    CAST_CHANNEL_TYPE_TO_STRING(ChallengeReplyError::NONE);
    CAST_CHANNEL_TYPE_TO_STRING(ChallengeReplyError::PEER_CERT_EMPTY);
    CAST_CHANNEL_TYPE_TO_STRING(ChallengeReplyError::WRONG_PAYLOAD_TYPE);
    CAST_CHANNEL_TYPE_TO_STRING(ChallengeReplyError::NO_PAYLOAD);
    CAST_CHANNEL_TYPE_TO_STRING(ChallengeReplyError::PAYLOAD_PARSING_FAILED);
    CAST_CHANNEL_TYPE_TO_STRING(ChallengeReplyError::MESSAGE_ERROR);
    CAST_CHANNEL_TYPE_TO_STRING(ChallengeReplyError::NO_RESPONSE);
    CAST_CHANNEL_TYPE_TO_STRING(ChallengeReplyError::FINGERPRINT_NOT_FOUND);
    CAST_CHANNEL_TYPE_TO_STRING(ChallengeReplyError::CERT_PARSING_FAILED);
    CAST_CHANNEL_TYPE_TO_STRING(
        ChallengeReplyError::CERT_NOT_SIGNED_BY_TRUSTED_CA);
    CAST_CHANNEL_TYPE_TO_STRING(ChallengeReplyError::CANNOT_EXTRACT_PUBLIC_KEY);
    CAST_CHANNEL_TYPE_TO_STRING(ChallengeReplyError::SIGNED_BLOBS_MISMATCH);
    CAST_CHANNEL_TYPE_TO_STRING(
        ChallengeReplyError::TLS_CERT_VALIDITY_PERIOD_TOO_LONG);
    CAST_CHANNEL_TYPE_TO_STRING(
        ChallengeReplyError::TLS_CERT_VALID_START_DATE_IN_FUTURE);
    CAST_CHANNEL_TYPE_TO_STRING(ChallengeReplyError::TLS_CERT_EXPIRED);
    CAST_CHANNEL_TYPE_TO_STRING(ChallengeReplyError::CRL_INVALID);
    CAST_CHANNEL_TYPE_TO_STRING(ChallengeReplyError::CERT_REVOKED);
    CAST_CHANNEL_TYPE_TO_STRING(ChallengeReplyError::SENDER_NONCE_MISMATCH);
    CAST_CHANNEL_TYPE_TO_STRING(ChallengeReplyError::SIGNATURE_EMPTY);
    CAST_CHANNEL_TYPE_TO_STRING(ChallengeReplyError::DIGEST_UNSUPPORTED);
  }
    NOTREACHED() << "Unknown challenge reply error "
               << ChallengeReplyErrorToString(challenge_error);
  return "Unknown challenge_error";
}

void ChannelOpened(int* channel_id_out,
                   bool* success,
                   base::WaitableEvent* event,
                   int channel_id,
                   ChannelError error) {
  *success = true;
  *channel_id_out = channel_id;
  if (error != ChannelError::NONE) {
    LOG(ERROR) << ChannelErrorToString(error);
    *success = false;
  }
  event->Signal();
}

void ConnectAsync(const std::string& ip_address_raw,
                  scoped_refptr<Logger> logger,
                  int* channel_id,
                  bool* success,
                  base::WaitableEvent* event) {
  net::IPAddress ip_address;
  CHECK(ip_address.AssignFromIPLiteral(ip_address_raw));
  net::IPEndPoint ip_endpoint(ip_address, static_cast<uint16_t>(kCastPort));
  CastSocketOpenParams params(ip_endpoint, new net::NetLog(),
                              base::TimeDelta::FromMilliseconds(0));

  AuthContext context = AuthContext::Create();
  CastSocket* socket = new CastSocketImpl(params, logger, context);
  socket->Connect(base::Bind(&ChannelOpened, channel_id, success, event));
}

bool Connect(const std::string& ip_address) {
  // Enable all features.
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeatures(
      {base::Feature{"CastSHA256Enforced", base::FEATURE_ENABLED_BY_DEFAULT},
       base::Feature{"CastNonceEnforced", base::FEATURE_ENABLED_BY_DEFAULT},
       base::Feature{"CastCertificateRevocation",
                     base::FEATURE_ENABLED_BY_DEFAULT}},
      {});

  std::unique_ptr<base::Thread> signing_thread =
      base::MakeUnique<base::Thread>("CastV2_Signing_Thread");
  signing_thread->StartWithOptions(
      base::Thread::Options(base::MessageLoop::TYPE_IO, 0));
  scoped_refptr<base::SingleThreadTaskRunner> signing_task_runner =
      signing_thread->task_runner();
  std::unique_ptr<base::WaitableEvent> event =
      base::MakeUnique<base::WaitableEvent>(
          base::WaitableEvent::ResetPolicy::MANUAL,
          base::WaitableEvent::InitialState::NOT_SIGNALED);
  scoped_refptr<Logger> logger(new Logger());
  int channel_id;
  bool success;

  signing_task_runner->PostTask(
      FROM_HERE, base::Bind(&ConnectAsync, ip_address, logger, &channel_id,
                            &success, base::Unretained(event.get())));
  event->Wait();

  if (!success) {
    LastError error = logger->GetLastError(channel_id);
    std::string challenge_error = ChallengeReplyErrorToString(error.challenge_reply_error);
    if (!challenge_error.empty())
      LOG(INFO) << challenge_error;
  }
  return success;
}

}  // namespace cast_channel

int main(int argc, const char** argv) {
  std::unique_ptr<base::AtExitManager> exit_manager(new base::AtExitManager());
  if (argc != 2) {
    LOG(ERROR) << "Must provide device IP address as argument.";
    return -1;
  }
  const char* ip_address = argv[1];
  const std::string address(ip_address);
  LOG(INFO) << "Connecting to device at IP: " << ip_address;
  bool success = cast_channel::Connect(ip_address);
  if (!success) {
    LOG(ERROR) << "Connection failed.";
    return -1;
  }
  LOG(INFO) << "Connection Succeeded.";
  return 0;
}